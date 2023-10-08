#include "vtkWasmRendererApplication.h"

#include <vtkActor.h>
#include <vtkCallbackCommand.h>
#include <vtkCamera.h>
#include <vtkCellData.h>
#include <vtkCharArray.h>
#include <vtkCollectionRange.h>
#include <vtkColorSeries.h>
#include <vtkColorTransferFunction.h>
#include <vtkGLTFReader.h>
#include <vtkGeometryFilter.h>
#include <vtkHardwarePicker.h>
#include <vtkHardwareSelector.h>
#include <vtkInteractorStyleRubberBandPick.h>
#include <vtkInteractorStyleSwitch.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkMemoryResourceStream.h>
#include <vtkMinimalStandardRandomSequence.h>
#include <vtkNew.h>
#include <vtkOBJReader.h>
#include <vtkOpenGLLowMemPolyDataMapper.h>
#include <vtkPLYReader.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyDataReader.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderedAreaPicker.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <vtkSTLReader.h>
#include <vtkSelectionNode.h>
#include <vtkSetGet.h>
#include <vtkShaderProgram.h>
#include <vtkSmartPointer.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkXMLUnstructuredGridReader.h>
#include <vtksys/SystemTools.hxx>

#include <emscripten.h>
#include <iostream>
#include <string>
#include <unordered_set>

namespace {
void EndPick(vtkObject *caller, unsigned long vtkNotUsed(eventId),
             void *clientData, void *callData) {
  auto interactor = vtkRenderWindowInteractor::SafeDownCast(caller);
  auto renderer =
      vtkRenderer::SafeDownCast(reinterpret_cast<vtkObject *>(clientData));
  vtkNew<vtkHardwareSelector> sel;
  sel->SetRenderer(renderer);

  double x0 = renderer->GetPickX1();
  double y0 = renderer->GetPickY1();
  double x1 = renderer->GetPickX2();
  double y1 = renderer->GetPickY2();

  sel->SetArea(static_cast<int>(x0), static_cast<int>(y0), static_cast<int>(x1),
               static_cast<int>(y1));
  vtkSmartPointer<vtkSelection> res;
  res.TakeReference(sel->Select());
  if (!res) {
    std::cerr << "Selection not supported." << endl;
    return;
  }

  std::cout << "x0 " << x0 << " y0 " << y0 << "\t";
  std::cout << "x1 " << x1 << " y1 " << y1 << endl;

  vtkSelectionNode *ids = res->GetNode(0);
  if (ids) {
    std::cout << "fieldType: " << ids->GetFieldTypeAsString(ids->GetFieldType())
              << std::endl;
    for (vtkIdType i = 0; i < ids->GetSelectionList()->GetNumberOfValues();
         ++i) {
      std::cout << "i : " << i << ": "
                << ids->GetSelectionList()->GetVariantValue(i) << std::endl;
    }
  }
}
} // namespace

class vtkWasmRendererApplication::Internal {
public:
  float ScrollSensitivity = 1.0;
  std::string ColorMapPreset = "Spectrum";

  std::unordered_set<std::string> PointDataArrays;
  std::unordered_set<std::string> CellDataArrays;

  void UpdateLUT();
  void FetchAvailableDataArrays(vtkDataSet *pointset);

  vtkNew<vtkActor> Actor;
  vtkNew<vtkRenderWindowInteractor> Interactor;
  vtkNew<vtkRenderWindow> Window;
  vtkNew<vtkRenderer> Renderer;
};

//------------------------------------------------------------------------------
vtkWasmRendererApplication::vtkWasmRendererApplication() {
  this->P = std::unique_ptr<Internal>(new Internal());
  std::cout << __func__ << std::endl;
  this->P->Window->SetWindowName("VTK WebAssembly Studio");
}

//------------------------------------------------------------------------------
vtkWasmRendererApplication::~vtkWasmRendererApplication() {
  std::cout << __func__ << std::endl;
}

//------------------------------------------------------------------------------
void vtkWasmRendererApplication::LoadDataFileFromMemory(
    const std::string &filename, std::uintptr_t buffer, std::size_t nbytes) {

  std::cout << __func__ << "(" << filename << ',' << buffer << ',' << nbytes
            << ")" << std::endl;

  vtkNew<vtkPolyDataMapper> mapper;
  vtkNew<vtkPolyData> mesh;

  auto wrappedBuffer = vtk::TakeSmartPointer(vtkBuffer<char>::New());
  wrappedBuffer->SetBuffer(reinterpret_cast<char *>(buffer), nbytes);
  wrappedBuffer->SetFreeFunction(/*noFreeFunction=*/true);

  using systools = vtksys::SystemTools;
  vtkNew<vtkMemoryResourceStream> stream;
  if (systools::StringEndsWith(filename, ".vtp")) {
    auto xmlreader = vtk::TakeSmartPointer(vtkXMLPolyDataReader::New());
    xmlreader->SetReadFromInputString(true);
    std::string inputString;
    inputString.assign(reinterpret_cast<char *>(buffer), nbytes);
    xmlreader->SetInputString(inputString);
    xmlreader->Update();
    mesh->ShallowCopy(xmlreader->GetOutput());
  } else if (systools::StringEndsWith(filename, ".vtu")) {
    auto xmlreader = vtk::TakeSmartPointer(vtkXMLUnstructuredGridReader::New());
    auto surface = vtk::TakeSmartPointer(vtkGeometryFilter::New());
    xmlreader->SetReadFromInputString(true);
    std::string inputString;
    inputString.assign(reinterpret_cast<char *>(buffer), nbytes);
    xmlreader->SetInputString(inputString);
    surface->SetInputConnection(xmlreader->GetOutputPort());
    surface->Update();
    mesh->ShallowCopy(surface->GetOutput());
  } else if (systools::StringEndsWith(filename, ".vtk")) {
    auto polydataReader = vtk::TakeSmartPointer(vtkPolyDataReader::New());
    polydataReader->ReadFromInputStringOn();
    vtkNew<vtkCharArray> wrappedArray;
    wrappedArray->SetArray(reinterpret_cast<char *>(buffer), nbytes, 1);
    polydataReader->SetInputArray(wrappedArray);
    polydataReader->Update();
    mesh->ShallowCopy(polydataReader->GetOutput());
  } else if (systools::StringEndsWith(filename, ".glb") ||
             systools::StringEndsWith(filename, ".gltf")) {
    // TODO: Hangs.
    // auto gltfreader = vtk::TakeSmartPointer(vtkGLTFReader::New());
    // mapper = vtk::TakeSmartPointer(vtkPolyDataMapper::New());
    // gltfreader->SetFileName(filename.c_str());
    // reader = gltfreader;
  } else if (systools::StringEndsWith(filename, ".obj")) {
    auto objreader = vtk::TakeSmartPointer(vtkOBJReader::New());
    stream->SetBuffer(wrappedBuffer);
    objreader->SetStream(stream);
    objreader->Update();
    mesh->ShallowCopy(objreader->GetOutput());
  } else if (systools::StringEndsWith(filename, ".ply")) {
    auto plyreader = vtk::TakeSmartPointer(vtkPLYReader::New());
    plyreader->SetReadFromInputStream(true);
    stream->SetBuffer(wrappedBuffer);
    plyreader->SetStream(stream);
    plyreader->Update();
    mesh->ShallowCopy(plyreader->GetOutput());
  }
  mapper->SetInputData(mesh);
  this->P->Actor->SetMapper(mapper);
  this->P->Renderer->AddActor(this->P->Actor);
  this->SetColorByArray("Solid");
  this->P->UpdateLUT();
  // render once so that the pipeline is updated
  this->P->Window->Render();
  // make the mapper static so that subsequent renders do not walk up the VTK
  // pipeline anymore.
  mapper->StaticOn();
  // Fetches point and cell data arrays from reader's output.
  this->P->FetchAvailableDataArrays(mesh);
}

//------------------------------------------------------------------------------
void vtkWasmRendererApplication::Azimuth(float value) {
  std::cout << __func__ << '(' << value << ')' << std::endl;
  this->P->Renderer->GetActiveCamera()->Azimuth(value);
  this->P->Renderer->ResetCameraClippingRange();
  std::cout << "Current memory usage " << (uintptr_t)sbrk(0) << " bytes";
}

//------------------------------------------------------------------------------
void vtkWasmRendererApplication::Initialize() {
  std::cout << __func__ << std::endl;
  this->P->Renderer->GradientBackgroundOn();
  this->P->Renderer->SetGradientMode(
      vtkRenderer::GradientModes::VTK_GRADIENT_RADIAL_VIEWPORT_FARTHEST_CORNER);
  this->P->Renderer->SetBackground(0.196, 0.298, 0.384);
  this->P->Renderer->SetBackground2(0.122, 0.114, 0.173);
  // create the default renderer
  this->P->Window->AddRenderer(this->P->Renderer);
  this->P->Window->SetInteractor(this->P->Interactor);
  // set the current style to TrackBallCamera. Default is joystick
  if (auto iStyle = vtkInteractorStyle::SafeDownCast(
          this->P->Interactor->GetInteractorStyle())) {
    if (auto switchStyle = vtkInteractorStyleSwitch::SafeDownCast(iStyle)) {
      switchStyle->SetCurrentStyleToTrackballCamera();
    }
  }
  // pass pick events to the HardwareSelector
  vtkNew<vtkInteractorStyleRubberBandPick> rbp;
  this->P->Interactor->SetInteractorStyle(rbp);
  vtkNew<vtkRenderedAreaPicker> areaPicker;
  this->P->Interactor->SetPicker(areaPicker);
  // pass pick events to the HardwareSelector
  vtkNew<vtkCallbackCommand> cbc;
  cbc->SetCallback(EndPick);
  cbc->SetClientData(this->P->Renderer);
  this->P->Interactor->AddObserver(vtkCommand::EndPickEvent, cbc);
  this->SetScrollSensitivity(0.15);
}

//------------------------------------------------------------------------------
void vtkWasmRendererApplication::Render() {
  std::cout << __func__ << std::endl;
  this->P->Window->Render();
}

//------------------------------------------------------------------------------
void vtkWasmRendererApplication::ResetView() {
  std::cout << __func__ << std::endl;
  auto ren = this->P->Window->GetRenderers()->GetFirstRenderer();
  if (ren != nullptr) {
    ren->ResetCamera();
  }
}

//------------------------------------------------------------------------------
void vtkWasmRendererApplication::RemoveAllActors() {
  std::cout << __func__ << std::endl;
  auto ren = this->P->Window->GetRenderers()->GetFirstRenderer();
  ren->RemoveAllViewProps();
  this->P->Window->Render();
}

//------------------------------------------------------------------------------
std::string
vtkWasmRendererApplication::SetVertexShaderSource(std::string source) {
  std::cout << __func__ << '(' << source.length() << ')' << std::endl;
  vtkOpenGLLowMemPolyDataMapper *lmpMapper =
      vtkOpenGLLowMemPolyDataMapper::SafeDownCast(this->P->Actor->GetMapper());
  if (lmpMapper) {
    if (auto program = lmpMapper->GetShaderProgram()) {
      program->GetVertexShader()->SetSource(source);
      if (!program->GetVertexShader()->Compile()) {
        return program->GetVertexShader()->GetError();
      }
    }
  }
  return "Success!";
}

//------------------------------------------------------------------------------
std::string
vtkWasmRendererApplication::SetFragmentShaderSource(std::string source) {
  std::cout << __func__ << '(' << source.length() << ')' << std::endl;
  vtkOpenGLLowMemPolyDataMapper *lmpMapper =
      vtkOpenGLLowMemPolyDataMapper::SafeDownCast(this->P->Actor->GetMapper());
  if (lmpMapper) {
    if (auto program = lmpMapper->GetShaderProgram()) {
      program->GetFragmentShader()->SetSource(source);
      if (!program->GetFragmentShader()->Compile()) {
        return program->GetFragmentShader()->GetError();
      }
    }
  }
  return "Success!";
}

//------------------------------------------------------------------------------
std::string vtkWasmRendererApplication::GetVertexShaderSource() {
  std::cout << __func__ << "()" << std::endl;
  vtkOpenGLLowMemPolyDataMapper *lmpMapper =
      vtkOpenGLLowMemPolyDataMapper::SafeDownCast(this->P->Actor->GetMapper());
  if (lmpMapper) {
    if (auto program = lmpMapper->GetShaderProgram()) {
      return program->GetVertexShader()->GetSource();
    }
  }
  return "Shader program not yet ready!";
}

//------------------------------------------------------------------------------
std::string vtkWasmRendererApplication::GetFragmentShaderSource() {
  std::cout << __func__ << "()" << std::endl;
  vtkOpenGLLowMemPolyDataMapper *lmpMapper =
      vtkOpenGLLowMemPolyDataMapper::SafeDownCast(this->P->Actor->GetMapper());
  if (lmpMapper) {
    if (auto program = lmpMapper->GetShaderProgram()) {
      return program->GetFragmentShader()->GetSource();
    }
  }
  return "Shader program not yet ready!";
}

//------------------------------------------------------------------------------
void vtkWasmRendererApplication::Start() {
  std::cout << __func__ << std::endl;
  this->P->Renderer->ResetCamera();
  this->P->Window->Render();
  this->P->Interactor->Start();
}

//------------------------------------------------------------------------------
void vtkWasmRendererApplication::Halt() { emscripten_pause_main_loop(); }

//------------------------------------------------------------------------------
void vtkWasmRendererApplication::Resume() { emscripten_resume_main_loop(); }

//------------------------------------------------------------------------------
void vtkWasmRendererApplication::SetScrollSensitivity(float sensitivity) {
  std::cout << __func__ << "(" << sensitivity << ")" << std::endl;
  if (auto iStyle = vtkInteractorStyle::SafeDownCast(
          this->P->Interactor->GetInteractorStyle())) {
    if (auto switchStyle = vtkInteractorStyleSwitch::SafeDownCast(iStyle)) {
      switchStyle->GetCurrentStyle()->SetMouseWheelMotionFactor(sensitivity);
    } else {
      iStyle->SetMouseWheelMotionFactor(sensitivity);
    }
  }
  this->P->ScrollSensitivity = sensitivity;
}

//------------------------------------------------------------------------------
void vtkWasmRendererApplication::SetRepresentation(int rep) {
  std::cout << __func__ << '(' << rep << ')' << std::endl;
  for (const auto &viewProp : vtk::Range(this->P->Renderer->GetViewProps())) {
    if (auto actor = static_cast<vtkActor *>(viewProp)) {
      if (rep <= VTK_SURFACE) {
        actor->GetProperty()->SetRepresentation(rep);
        actor->GetProperty()->SetEdgeVisibility(false);
      } else if (rep == 3) {
        actor->GetProperty()->SetRepresentationToSurface();
        actor->GetProperty()->SetEdgeVisibility(true);
      }
    }
  }
  this->P->Window->Render();
}

//------------------------------------------------------------------------------
void vtkWasmRendererApplication::SetVertexVisibility(bool visible) {
  std::cout << __func__ << '(' << visible << ')' << std::endl;
  this->P->Actor->GetProperty()->SetVertexVisibility(visible);
}

//------------------------------------------------------------------------------
void vtkWasmRendererApplication::SetPointSize(float value) {
  std::cout << __func__ << '(' << value << ')' << std::endl;
  this->P->Actor->GetProperty()->SetPointSize(value);
}

//------------------------------------------------------------------------------
void vtkWasmRendererApplication::SetLineWidth(float value) {
  std::cout << __func__ << '(' << value << ')' << std::endl;
  this->P->Actor->GetProperty()->SetLineWidth(value);
}

//------------------------------------------------------------------------------
void vtkWasmRendererApplication::SetColorByArray(const std::string &arrayName) {
  auto mapper = this->P->Actor->GetMapper();
  if (!mapper) {
    return;
  }
  std::cout << __func__ << '(' << arrayName << ')' << std::endl;
  if (arrayName == "Solid") {
    mapper->ScalarVisibilityOff();
    return;
  }
  mapper->ScalarVisibilityOn();
  vtkDataArray *scalarArray = nullptr;
  if (this->P->PointDataArrays.count(arrayName)) {
    mapper->SetScalarModeToUsePointFieldData();
    scalarArray = mapper->GetInputAsDataSet()->GetPointData()->GetArray(
        arrayName.c_str());
  } else if (this->P->PointDataArrays.count(arrayName)) {
    mapper->SetScalarModeToUseCellFieldData();
    scalarArray =
        mapper->GetInputAsDataSet()->GetCellData()->GetArray(arrayName.c_str());
  } else {
    return;
  }
  // TODO: handle multi-component arrays with magnitude, x, y, z ..
  mapper->ColorByArrayComponent(arrayName.c_str(), 0);
  if (scalarArray) {
    double range[2];
    scalarArray->GetRange(range);
    mapper->SetScalarRange(range);
  }
  this->P->UpdateLUT();
}

//------------------------------------------------------------------------------
void vtkWasmRendererApplication::SetInterpolateScalarsBeforeMapping(
    bool value) {
  std::cout << __func__ << '(' << value << ')' << std::endl;
  if (auto mapper = this->P->Actor->GetMapper()) {
    mapper->SetInterpolateScalarsBeforeMapping(value);
  }
}

//------------------------------------------------------------------------------
void vtkWasmRendererApplication::SetColor(int r, int g, int b) {
  std::cout << __func__ << '(' << r << ',' << g << ',' << b << ')' << std::endl;
  this->P->Actor->GetProperty()->SetColor(r / 255., g / 255., b / 255.);
}

//------------------------------------------------------------------------------
void vtkWasmRendererApplication::SetColorMapPreset(
    const std::string &presetName) {
  std::cout << __func__ << '(' << presetName << ')' << std::endl;
  this->P->ColorMapPreset = presetName;
  this->P->UpdateLUT();
}

//------------------------------------------------------------------------------
void vtkWasmRendererApplication::SetEdgeColor(int r, int g, int b) {
  std::cout << __func__ << '(' << r << ',' << g << ',' << b << ')' << std::endl;
  this->P->Actor->GetProperty()->SetEdgeColor(r / 255., g / 255., b / 255.);
}

//------------------------------------------------------------------------------
void vtkWasmRendererApplication::SetVertexColor(int r, int g, int b) {
  std::cout << __func__ << '(' << r << ',' << g << ',' << b << ')' << std::endl;
  this->P->Actor->GetProperty()->SetVertexColor(r / 255., g / 255., b / 255.);
}

//------------------------------------------------------------------------------
void vtkWasmRendererApplication::SetOpacity(float value) {
  std::cout << __func__ << '(' << value << ')' << std::endl;
  this->P->Actor->GetProperty()->SetOpacity(value);
}

//------------------------------------------------------------------------------
void vtkWasmRendererApplication::SetEdgeOpacity(float value) {
  std::cout << __func__ << '(' << value << ')' << std::endl;
  this->P->Actor->GetProperty()->SetEdgeOpacity(value);
}

//------------------------------------------------------------------------------
std::string vtkWasmRendererApplication::GetPointDataArrays() {
  std::cout << __func__ << std::endl;
  std::string result;
  for (auto &arrayName : this->P->PointDataArrays) {
    result = result + arrayName + ';';
  }
  // remove that last ';'
  return result.substr(0, result.length() - 1);
}

//------------------------------------------------------------------------------
std::string vtkWasmRendererApplication::GetCellDataArrays() {
  std::cout << __func__ << std::endl;
  std::string result;
  for (auto &arrayName : this->P->CellDataArrays) {
    result = result + arrayName + ';';
  }
  // remove that last ';'
  return result.substr(0, result.length() - 1);
}

namespace {
std::map<std::string, int> PresetNames = {
    {"Spectrum", vtkColorSeries::SPECTRUM},
    {"Warm", vtkColorSeries::WARM},
    {"Cool", vtkColorSeries::COOL},
    {"Blues", vtkColorSeries::BLUES},
    {"WildFlower", vtkColorSeries::WILD_FLOWER},
    {"Citrus", vtkColorSeries::CITRUS}};
} // namespace

//------------------------------------------------------------------------------
std::string vtkWasmRendererApplication::GetColorMapPresets() {
  return "Spectrum;Warm;Cool;Blues;WildFlower;Citrus";
}

//------------------------------------------------------------------------------
void vtkWasmRendererApplication::Internal::UpdateLUT() {
  auto mapper = this->Actor->GetMapper();
  if (!mapper) {
    return;
  }
  std::array<double, 2> scalarRange;
  mapper->GetScalarRange(scalarRange.data());
  vtkNew<vtkColorTransferFunction> lut;
  lut->SetColorSpaceToHSV();
  vtkNew<vtkColorSeries> colorSeries;
  colorSeries->SetColorScheme(PresetNames.at(this->ColorMapPreset));
  auto numColors = colorSeries->GetNumberOfColors();
  for (int i = 0; i < numColors; ++i) {
    vtkColor3ub color = colorSeries->GetColor(i);
    double dColor[3];
    dColor[0] = static_cast<double>(color[0]) / 255.0;
    dColor[1] = static_cast<double>(color[1]) / 255.0;
    dColor[2] = static_cast<double>(color[2]) / 255.0;
    double t = scalarRange[0] + (scalarRange[1] - scalarRange[0]) /
                                    (static_cast<double>(numColors) - 1) * i;
    lut->AddRGBPoint(t, dColor[0], dColor[1], dColor[2]);
  }
  mapper->SetLookupTable(lut);
}

//------------------------------------------------------------------------------
void vtkWasmRendererApplication::Internal::FetchAvailableDataArrays(
    vtkDataSet *dataSet) {
  this->PointDataArrays.clear();
  if (auto pointData = dataSet->GetPointData()) {
    for (int i = 0; i < pointData->GetNumberOfArrays(); ++i) {
      this->PointDataArrays.insert(pointData->GetArrayName(i));
    }
  }
  this->CellDataArrays.clear();
  if (auto cellData = dataSet->GetCellData()) {
    for (int i = 0; i < cellData->GetNumberOfArrays(); ++i) {
      this->CellDataArrays.insert(cellData->GetArrayName(i));
    }
  }
}