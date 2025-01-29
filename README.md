# VTK WebAssembly Studio

This project allows you to develop a minimal VTK application in a web browser.
The code from the browser is sent to a build server for compiling with VTK.wasm SDK.
The server pulls a kitware/vtk-wasm-sdk docker image and builds the code. Finally, the
build artifact is sent back to the client and displayed in a panel using an `iframe`.

## Usage

Start the build server:

```bash
python -m pip install -r server/requirements.txt
python server/main.py
======== Running on http://0.0.0.0:8080 ========
```

Start the client:

```bash
cd client
npm i
npm run dev
<i> [webpack-dev-server] Project is running at:
<i> [webpack-dev-server] Loopback: http://localhost:8081/, http://[::1]:8081/
```
