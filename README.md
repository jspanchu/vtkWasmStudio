# vtkWasmStudio
VTK WebAssembly Studio

Generate `static/vtkWasmRendererApplication.js` by building the C++ project with CMake.

```sh
mkdir out
emcmake cmake -GNinja -S . -B out -DVTK_DIR=/path/to/VTK
cmake --build out
```
Note: The C++ code relies upon `vtkOpenGLLowMemPolyDataMapper` which is not yet in any VTK branch. So it currently won't compile.

Then setup a python virtual environment, install requirements and run `driver.py`
