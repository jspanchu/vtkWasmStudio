from aiohttp import web
from aiohttp import ContentTypeError
from base64 import b64encode, b64decode
from pathlib import Path
from pydantic import BaseModel
from pydantic import ValidationError
from shutil import rmtree

import aiohttp_cors
import docker
import logging
import tempfile

client = docker.from_env()


logger = logging.getLogger('vtk-wasm-sdk-build-server')
logging.basicConfig(level=logging.DEBUG,
                    style='{', format='{asctime} {levelname} {message}')


def encode(name: str) -> str:
    return b64encode(
        name.encode(encoding='utf-8')).decode(encoding='utf-8')


def decode(hash_value: str) -> str:
    return b64decode(
        hash_value.encode(encoding='utf-8')).decode(encoding='utf-8')


class DockerImageInfo(BaseModel):
    repository: str
    tag: str


class SourceInfo(BaseModel):
    name: str
    content: str


class BuilderInfo(BaseModel):
    image: DockerImageInfo
    sources: list[SourceInfo]
    config: str


routes = web.RouteTableDef()


def run(image: str, mounts: [docker.types.Mount], command: [str]) -> str:

    logger.info(f"run {command=}")
    full_command = ' '.join(command)
    logs = client.containers.run(
        image=image, remove=True, detach=False, mounts=mounts, command=full_command)

    logs = '\n'.join([full_command, logs.decode(encoding='utf-8')])
    logger.info(logs)
    return logs


@routes.post('/build')
async def build_handler(request: web.Request):

    try:
        data = await request.json()
    except ContentTypeError:
        return web.Response(text='Invalid JSON', status=400)

    logger.debug(f'build {data=}')

    try:
        build_info = BuilderInfo(**data)
        logger.debug(f'{build_info=}')
    except ValidationError as e:
        logger.error(f'{e.errors()}')
        return web.json_response(e.errors(), status=400)

    try:
        client.images.get(build_info.image.repository)
        logger.debug(f'present {build_info.image.repository=}')
    except docker.errors.ImageNotFound:
        try:
            logger.debug(f'pull {build_info.image=}')
            client.images.pull(build_info.image.repository,
                               build_info.image.tag)
        except docker.errors.APIError as e:
            return web.json_response({'error': str(e)}, status=400)

    tmp_dir = tempfile.mkdtemp(prefix='vtk_wasm_sdk_server')

    # create files for sources and populate content
    for source_info in build_info.sources:
        source_path = Path(tmp_dir).joinpath(source_info.name)
        with source_path.open(mode='w') as fh:
            fh.write(source_info.content)
        logger.info(f"Wrote {source_path}")

    image = build_info.image.repository + ':' + build_info.image.tag

    # will be used to store build artifacts so that client can retrieve in GET
    unique_key = Path(tmp_dir).stem

    target_dir = Path('/').joinpath(unique_key)
    build_dir = target_dir.joinpath("build")

    mounts = [
        docker.types.Mount(f'{target_dir.as_posix()}', tmp_dir, type='bind')
    ]

    logs = ""
    # Run CMake configure
    try:
        logs += run(image, mounts, [
            "emcmake",
            "cmake",
            # source directory
            "-S",
            target_dir.as_posix(),
            # build directory
            "-B",
            build_dir.as_posix(),
            f"-DCMAKE_BUILD_TYPE={build_info.config}",
            f"-DVTK_DIR=/VTK-install/{build_info.config}/lib/cmake/vtk",
        ])
    except docker.errors.ContainerError as e:
        logger.error(str(e))
        return web.json_response({'error': str(e)}, status=400)

    # Run CMake build
    try:
        logs += run(image, mounts, [
            "cmake",
            "--build",
            build_dir.as_posix(),
        ])
    except docker.errors.ContainerError as e:
        logger.error(str(e))
        return web.json_response({'error': str(e)}, status=400)

    return web.json_response({'id': encode(tmp_dir), 'logs': logs})


@routes.get('/{filename}')
async def get_handler(request: web.Request):

    filename = request.match_info['filename']

    artifacts_dir_hash = request.query.get('id', "")
    artifacts_dir_name = decode(artifacts_dir_hash)

    logger.info(f'get {filename=} {artifacts_dir_name=}')

    if len(artifacts_dir_name):
        file_path = Path(artifacts_dir_name).joinpath(
            'build').joinpath(filename)
        if file_path.exists():
            return web.FileResponse(file_path)
        else:
            return web.Response(text=f"File {filename} does not exist in {artifacts_dir_name}", status=400)
    else:
        return web.Response(text=f"Invalid directory name for id={artifacts_dir_name}", status=400)

    return web.Response(text='Ok', status=200)


@routes.delete('/delete')
async def delete_handler(request):

    artifacts_dir_hash = request.query.get('id', "")
    artifacts_dir_name = decode(artifacts_dir_hash)

    logger.info(f'delete {artifacts_dir_name=}')
    rmtree(artifacts_dir_name)

    return web.Response(text='Ok', status=200)


if __name__ == '__main__':
    app = web.Application()
    app.add_routes(routes)

    # Configure default CORS settings.
    cors = aiohttp_cors.setup(app, defaults={
        "*": aiohttp_cors.ResourceOptions(
            allow_credentials=True,
            expose_headers="*",
            allow_headers="*",
        )
    })

    # Configure CORS on all routes.
    for route in list(app.router.routes()):
        cors.add(route)
    web.run_app(app)
