
export const htmlContent =
    "<!doctype html>\n" +
    "<html>\n" +
    "<head>\n" +
    "  <meta charset='utf-8' />\n" +
    "</head>\n" +
    "<body>\n" +
    "  <canvas id='canvas' style='position: absolute; left: 0; top: 0;'></canvas>\n" +
    "  <script type='text/javascript'>\n" +
    "    var Module = {\n" +
    "      'preRun': [(runtime) => {\n" +
    "        // cache the VTK wasm runtime instance.\n" +
    "        vtkWasmRuntime = runtime;\n" +
    "      }],\n" +
    "      'canvas': (function () {\n" +
    "        var canvas = document.getElementById('canvas');\n" +
    "        canvas.addEventListener(\n" +
    "          'webglcontextlost',\n" +
    "          function (e) {\n" +
    "            console.error('WebGL context lost. You will need to reload the page.');\n" +
    "            e.preventDefault();\n" +
    "          },\n" +
    "          false\n" +
    "        );\n" +
    "        return canvas;\n" +
    "      })(),\n" +
    "      'print': (function () {\n" +
    "        return function (text) {\n" +
    "          text = Array.prototype.slice.call(arguments).join(' ');\n" +
    "          console.info(text);\n" +
    "        };\n" +
    "      })(),\n" +
    "      'printErr': function (text) {\n" +
    "        text = Array.prototype.slice.call(arguments).join(' ');\n" +
    "        console.error(text);\n" +
    "      },\n" +
    "      'onRuntimeInitialized': function () {\n" +
    "        console.log('WASM runtime initialized');\n" +
    "        // sends a resize event so that the render window fills up browser tab dimensions.\n" +
    "        setTimeout(() => {\n" +
    "          window.dispatchEvent(new Event('resize'));\n" +
    "        }, 0);\n" +
    "        // focus on the canvas to grab keyboard inputs.\n" +
    "        canvas.setAttribute('tabindex', '0');\n" +
    "        // grab focus when the render window region receives mouse clicks.\n" +
    "        canvas.addEventListener('click', () => canvas.focus());\n" +
    "      },\n" +
    "    };\n" +
    "  </script>\n" +
    "  {{{ SCRIPT }}}\n" +
    "</body>\n" +
    "</html>\n";
