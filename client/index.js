import * as monaco from "monaco-editor/esm/vs/editor/editor.api";
import { Terminal } from '@xterm/xterm';
import { FitAddon } from '@xterm/addon-fit';
import Split from "split-grid";
import { spinner, previewTemplate, miniBrowserTemplate } from "./preview-template.mjs";

import { render, html } from "lit";

import { cmakeContent } from "./cmakeContent";

import "./style.css";
import { htmlContent } from "./htmlContent";

const editorContainer = document.createElement("div");
const editor = monaco.editor.create(editorContainer, {
    value: "",
    language: "cpp",
    theme: "vs-dark",
});

const terminalContainer = document.createElement("div");
const terminal = new Terminal({
    convertEol: true,
    theme: {
        background: "#1e1e1e",
        foreground: "#d4d4d4",
    },
});
terminal.open(terminalContainer);

const terminalFitAddon = new FitAddon();
terminal.loadAddon(terminalFitAddon);

window.editor = editor;
window.terminal = terminal;

editor.setValue(`// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkConeSource.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkNew.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

//------------------------------------------------------------------------------
// Main
//------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
  // Create a renderer, render window, and interactor
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetMultiSamples(0);
  renderWindow->AddRenderer(renderer);
  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);

  vtkNew<vtkInteractorStyleTrackballCamera> style;
  renderWindowInteractor->SetInteractorStyle(style);
  style->SetDefaultRenderer(renderer);

  // Create pipeline
  vtkNew<vtkConeSource> coneSource;
  coneSource->Update();

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(coneSource->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetEdgeVisibility(1);
  actor->GetProperty()->SetEdgeColor(1, 0, 1);

  // Add the actors to the scene
  renderer->AddActor(actor);

  // Start rendering app
  renderer->SetBackground(0.2, 0.3, 0.4);
  renderWindow->SetSize(300, 300);
  renderWindow->Render();

  // Start event loop
  renderWindowInteractor->Start();

  return 0;
}
`);

window.addEventListener("resize", () => {
    editor.layout();
    terminalFitAddon.fit();
});

async function main() {
    render(html`
        <div id="layout">
            <div id="header">
                <div id="title">VTK WebAssembly Studio</div>
                <input id="server" type="text"></input>
                <input id="repo" type="text"></input>
                <input id="tag" type="text"></input>
                <select name="config" id="config">
                    <option value="Debug">Debug</option>
                    <option value="Release">Release</option>
                    <option value="RelWithDebInfo">RelWithDebInfo</option>
                </select>
                <button disabled id="build">Loading</button>
            </div>
            <div id="editor">${editorContainer}</div>
            <div id="vgutter"></div>
            <div id="preview">
                <iframe id="preview-frame"></iframe>
            </div>
            <div id="hgutter"></div>
            <div id="output">
                <div id="terminal">
                    ${terminalContainer}
                </div>
                <div id="status"></div>
            </div>
        </div>
    `, document.body);

    const server = document.getElementById("server");
    server.value = "http://localhost:8080";

    const repo = document.getElementById("repo");
    repo.value = "kitware/vtk-wasm-sdk";

    const tag = document.getElementById("tag");
    tag.value = "wasm32-v9.4.1-1059-g71e59d05f0-20250125";

    const config = document.getElementById("config");
    config.value = "Release";

    window.split = Split({
        onDrag: () => {
            editor.layout();
            terminalFitAddon.fit();
        },
        columnGutters: [{
            track: 1,
            element: document.getElementById("vgutter"),
        }],
        rowGutters: [{
            track: 2,
            element: document.getElementById("hgutter"),
        }],
    });

    const frame = document.getElementById("preview-frame");
    let url = "";
    function preview(html_content) {
        if (url) URL.revokeObjectURL(url);
        url = URL.createObjectURL(new Blob([html_content], { type: 'text/html' }));
        frame.src = url;
    }

    let miniUrl = "";
    function previewMiniBrowser(html_blob) {
        if (miniUrl) URL.revokeObjectURL(miniUrl);
        miniUrl = URL.createObjectURL(html_blob);
        preview(miniBrowserTemplate("main.html", miniUrl));
    }

    preview(previewTemplate(spinner(80), "Loading", ""));

    const status = document.getElementById("status");
    const build = document.getElementById("build");
    build.addEventListener("click", async () => {
        build.disabled = true;
        build.textContent = "Compiling";
        status.textContent = `Compiling on ${server.value}`;
        preview(previewTemplate(spinner(80), "Compiling", ""));
        try {
            terminal.reset();
            let response = await fetch(server.value + '/build', {
                method: 'POST',
                body: JSON.stringify({
                    config: config.value,
                    image: {
                        repository: repo.value,
                        tag: tag.value,
                    },
                    sources: [
                        {
                            name: "main.cpp",
                            content: editor.getValue(),
                        },
                        {
                            name: "CMakeLists.txt",
                            content: cmakeContent,
                        },
                        {
                            name: "shell.html",
                            content: htmlContent,
                        },
                    ],
                }),

            });
            terminal.write("\n");
            if (!response.ok) {
                throw new Error(`Response status: ${response.status}`);
            }
            const result = await response.json();
            if (result.id !== undefined) {
                for (const line of result.logs.split('\\n')) {
                    terminal.write(line + "\n");
                }
                terminal.write("compilation finished");
                response = await fetch(server.value + '/main.html?' + new URLSearchParams({ id: result.id }).toString(), {
                    method: 'GET',
                });
                if (!response.ok) {
                    throw new Error(`Response status: ${response.status}`);
                }
                let blob = await response.blob();
                previewMiniBrowser(blob);
                // delete file
                response = await fetch(server.value + '/delete?' + new URLSearchParams({ id: result.id }).toString(), {
                    method: 'DELETE'
                });
                if (!response.ok) {
                    throw new Error(`Response status: ${response.status}`);
                }
            } else {
                for (const line of result.error.split('\\n')) {
                    terminal.write(line + "\n");
                }
                terminal.write(`compilation failed`);
                preview(previewTemplate("", "", "The compilation failed, check the output bellow"));
            }
            terminal.write("\n");
            terminalFitAddon.fit();
        } catch (err) {
            preview(previewTemplate("", "", "Something went wrong, please file a bug report"));
            console.error(err);
        } finally {
            status.textContent = "Idle";
            build.textContent = "Compile";
            build.disabled = false;
        }
    });

    requestAnimationFrame(() => {
        requestAnimationFrame(() => {
            editor.layout();
            terminalFitAddon.fit();
        });
    });
    terminal.write("Loading...\n");
    status.textContent = "Loading...";

    terminal.reset();
    terminal.write("Ready\n");
    status.textContent = "Idle";
    build.disabled = false;
    build.textContent = "Compile";
    preview(previewTemplate("", "", "<div>Code will run here.</div><div>Click <div style=\"display: inline-block;border: 1px solid #858585;background: #454545;color: #cfcfcf;font-size: 15px;padding: 5px 10px;border-radius: 3px;\">Compile</div> above to start.</div>"));
}

main();
