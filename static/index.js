import { chunkify } from './Chunkify.js';
import { maxSize } from './ArrayBufferLimits.js';
import Module from './vtkWasmRendererApplication.js';

var MAX_INT53 = 9007199254740992;
var MIN_INT53 = -9007199254740992;
var bigintToI53Checked = num => (num < MIN_INT53 || num > MAX_INT53) ? NaN : Number(num);

var animationRequestId = null;

function spinABit() {
  wasmApplication.azimuth(1);
  wasmApplication.render();
  animationRequestId = requestAnimationFrame(spinABit)
}

function startAnimation() {
  spinABit();
}

function stopAnimation() {
  console.log("cancel " + animationRequestId)
  cancelAnimationFrame(animationRequestId)
}

var args = {
  canvas: (function () {
    var el = document.getElementById('canvas');
    el.addEventListener(
      'webglcontextlost',
      function (e) {
        console.error('WebGL context lost. You will need to reload the page.');
        e.preventDefault();
      },
      false
    );
    return el;
  })(),
  print: (function () {
    return function (text) {
      text = Array.prototype.slice.call(arguments).join(' ');
      console.info(text);
    };
  })(),
  printErr: function (text) {
    text = Array.prototype.slice.call(arguments).join(' ');
    console.error(text);
  },
  onRuntimeInitialized: async function () {
    console.debug('WASM runtime initialized');
  }
};

/// setup canvas
let canvas = document.getElementById('canvas');
canvas.oncontextmenu = (event) => { event.preventDefault(); };
canvas.onclick = () => { canvas.focus(); }; // grab focus when the render window region receives mouse clicks.
canvas.tabIndex = -1;

// Start the wasm runtime
let wasmRuntime = await Module(args);
// Initialize application
let wasmApplication = new wasmRuntime.vtkWasmRendererApplication();

function setServerSideColorArrays() {
  var colorArrays = ['Solid', ...wasmApplication.getPointDataArrays().split(';'), ...wasmApplication.getCellDataArrays().split(';')];
  globalThis.parent.trame.state.set('color_modes', colorArrays.filter((el) => { return el.length > 0; }));
}

function setServerSideColorPresets() {
  globalThis.parent.trame.state.set('color_presets', wasmApplication.getColorMapPresets().split(';'));
}

async function loadFiles(files) {
  wasmApplication.removeAllActors();
  if (!files.length) {
    // clear color arrays
    setServerSideColorArrays();
    return;
  }
  let file = files[0];
  let name = file.name;
  let chunks = chunkify(file, /*chunkSize=*/bigintToI53Checked(maxSize))
  let offset = 0;
  let ptr = wasmRuntime._malloc(file.size);
  for (let i = 0; i < chunks.length; ++i) {
    let chunk = chunks[i];
    let data = new Uint8Array(await chunk.arrayBuffer());
    wasmRuntime.HEAPU8.set(data, ptr + offset);
    offset += data.byteLength;
  }
  wasmApplication.loadDataFileFromMemory(name, ptr, file.size);
  wasmRuntime._free(ptr);
  wasmApplication.resetView();
  wasmApplication.render();
  // populate color arrays
  setServerSideColorArrays();
}

// send presets to server
setServerSideColorPresets();
// initializes the render window, interactor, adds actors into the renderer.
wasmApplication.initialize();
// reset camera view.
wasmApplication.resetView();
// render once.
wasmApplication.render();
// sends a resize event so that the render window fills up browser tab dimensions.
setTimeout(() => {
  window.dispatchEvent(new Event('resize'));
}, 0);
// starts processing events on browser main thread.
wasmApplication.start();

globalThis.wasmApplication = wasmApplication;
globalThis.loadFiles = loadFiles;
globalThis.startAnimation = startAnimation;
globalThis.stopAnimation = stopAnimation;
globalThis.parent.trame.state.set('wasm_application_initialized', true);
