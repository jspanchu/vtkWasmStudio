#pragma once

#include <memory>
#include <string>

class vtkDataSet;
class vtkWasmRendererApplication {
public:
  vtkWasmRendererApplication();
  ~vtkWasmRendererApplication();

  void LoadDataFileFromMemory(const std::string &filename,
                              std::uintptr_t buffer, std::size_t nbytes);

  void Initialize();
  void Render();
  void ResetView();
  void RemoveAllActors();

  std::string SetVertexShaderSource(std::string source);
  std::string SetFragmentShaderSource(std::string source);
  std::string GetVertexShaderSource();
  std::string GetFragmentShaderSource();

  void Start();
  void Halt();
  void Resume();

  void SetScrollSensitivity(float sensitivity);

  void Azimuth(float value);

  void SetRepresentation(int representation);
  void SetVertexVisibility(bool visible);
  void SetPointSize(float value);
  void SetLineWidth(float value);

  void SetColorByArray(const std::string &arrayName);
  void SetInterpolateScalarsBeforeMapping(bool value);
  void SetColor(int r, int g, int b);
  void SetColorMapPreset(const std::string &presetName);
  void SetVertexColor(int r, int g, int b);
  void SetEdgeColor(int r, int g, int b);
  void SetOpacity(float value);
  void SetEdgeOpacity(float value);

  std::string GetPointDataArrays();
  std::string GetCellDataArrays();
  std::string GetColorMapPresets();

private:
  class Internal;
  std::unique_ptr<Internal> P;
};

#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>
EMSCRIPTEN_BINDINGS(vtkWasmRendererApplicationJSBindings) {
  emscripten::class_<vtkWasmRendererApplication>("vtkWasmRendererApplication")
      .constructor<>()
      .function("loadDataFileFromMemory",
                &vtkWasmRendererApplication::LoadDataFileFromMemory)
      .function("initialize", &vtkWasmRendererApplication::Initialize)
      .function("render", &vtkWasmRendererApplication::Render)
      .function("resetView", &vtkWasmRendererApplication::ResetView)
      .function("removeAllActors", &vtkWasmRendererApplication::RemoveAllActors)
      .function("setVertexShaderSource", &vtkWasmRendererApplication::SetVertexShaderSource)
      .function("setFragmentShaderSource", &vtkWasmRendererApplication::SetFragmentShaderSource)
      .function("getVertexShaderSource", &vtkWasmRendererApplication::GetVertexShaderSource)
      .function("getFragmentShaderSource", &vtkWasmRendererApplication::GetFragmentShaderSource)
      .function("start", &vtkWasmRendererApplication::Start)
      .function("halt", &vtkWasmRendererApplication::Halt)
      .function("resume", &vtkWasmRendererApplication::Resume)
      .function("azimuth", &vtkWasmRendererApplication::Azimuth)
      .function("setRepresentation",
                &vtkWasmRendererApplication::SetRepresentation)
      .function("setVertexVisibility",
                &vtkWasmRendererApplication::SetVertexVisibility)
      .function("setPointSize", &vtkWasmRendererApplication::SetPointSize)
      .function("setLineWidth", &vtkWasmRendererApplication::SetLineWidth)
      .function("setColorByArray", &vtkWasmRendererApplication::SetColorByArray)
      .function("setInterpolateScalarsBeforeMapping", &vtkWasmRendererApplication::SetInterpolateScalarsBeforeMapping)
      .function("setColor", &vtkWasmRendererApplication::SetColor)
      .function("setColorMapPreset",
                &vtkWasmRendererApplication::SetColorMapPreset)
      .function("setVertexColor", &vtkWasmRendererApplication::SetVertexColor)
      .function("setEdgeColor", &vtkWasmRendererApplication::SetEdgeColor)
      .function("setOpacity", &vtkWasmRendererApplication::SetOpacity)
      .function("setEdgeOpacity", &vtkWasmRendererApplication::SetEdgeOpacity)
      .function("getPointDataArrays",
                &vtkWasmRendererApplication::GetPointDataArrays)
      .function("getCellDataArrays",
                &vtkWasmRendererApplication::GetCellDataArrays)
      .function("getColorMapPresets",
                &vtkWasmRendererApplication::GetColorMapPresets);
}
#endif
