r"""
Installation requirements:
    pip install trame trame-vuetify trame-plotly
"""

from pathlib import Path
from trame.app import get_server
from trame.ui.vuetify2 import SinglePageWithDrawerLayout
from trame.widgets import client, code, html, vuetify
from trame.decorators import TrameApp, change, controller, life_cycle, hot_reload


class Representation:
    Points = 0
    Wireframe = 1
    Surface = 2
    SurfaceWithEdges = 3


class ShaderTypes:
    Vertex = 0
    Fragment = 0


SHADER_TYPES = [
    ("Vertex", ShaderTypes.Vertex),
    ("Fragment", ShaderTypes.Fragment)
]

SHADER_GET_METHODS = [
    "getVertexShaderSource",
    "getFragmentShaderSource",
]

SHADER_SET_METHODS = [
    "setVertexShaderSource",
    "setFragmentShaderSource"
]

WASM_WWW = str(Path(__file__).with_name("static"))


@TrameApp()
class Driver():
    def __init__(self, server=None):
        self.server = get_server(server, client_type="vue2", ws_heart_beat=1000)
        self.server.enable_module(dict(serve={"wasm": WASM_WWW}))
        self.server.state.trame__title = "VTK WebAssembly Studio"
        self._wasm_view_card = None
        self.ui()

    @property
    def state(self):
        return self.server.state

    @property
    def ctrl(self):
        return self.server.controller

    @property
    def wasm_application_ready(self):
        return self.state.wasm_application_initialized

    def call_js_method(self, method, *args):
        self.ctrl.call_js_method(
            {"method": method, "args": args})

    def call_js_method_return_value(self, stateVar, method, *args):
        self.ctrl.call_js_method_return_value(
            {"var": stateVar, "method": method, "args": args})

    def call_wasm_application_method(self, method, *args, needs_wasm_runtime=True):
        # Ignore requests to call WASM/JS when wasm runtime is not yet ready.
        if needs_wasm_runtime and not self.wasm_application_ready:
            return
        self.ctrl.call_wasm_application_method(
            {"method": method, "args": args})

    def call_wasm_application_method_return_value(self, stateVar, method, *args, needs_wasm_runtime=True):
        # Ignore requests to call WASM/JS when wasm runtime is not yet ready.
        if needs_wasm_runtime and not self.wasm_application_ready:
            return
        self.ctrl.call_wasm_application_method_return_value(
            {"var": stateVar, "method": method, "args": args})

    @property
    def representations(self):
        return [
            {"text": "Points", "value": Representation.Points},
            {"text": "Wireframe", "value": Representation.Wireframe},
            {"text": "Surface", "value": Representation.Surface},
            {"text": "Surface With Edges", "value": Representation.SurfaceWithEdges},
        ]

    # Actions -----------------------------------------------------------------
    def update_client_views(self):
        """
        Updates vtk render view and also pulls shader source from VTK into the active code editor tab.
        """
        # Render once so that shader programs get initialized by VTK.
        self.update_client_vtk_view()
        # Fetch shader code and display in editor.
        self.update_client_shader_view()

    def update_client_vtk_view(self):
        """
        Updates vtk render view.
        """
        self.call_wasm_application_method("render")

    def update_client_shader_view(self):
        """
        Pulls shader source code from VTK into the active code editor tab.
        """
        self._block_shader_set = True
        getter = SHADER_GET_METHODS[self.current_editor_tab_idx]
        self.call_wasm_application_method_return_value(
            'editor_content', getter)
        self._block_shader_set = False

    def reset_camera(self):
        """
        Reset camera to bounds.
        """
        self.call_wasm_application_method("resetView")
        self.update_client_vtk_view()

    @change("spin_view")
    def on_spin_view_change(self, spin_view, **kwargs):
        """
        Rotates the 3D view.
        """
        if spin_view:
            self.call_js_method("startAnimation")
        else:
            self.call_js_method("stopAnimation")

    @change("wasm_application_initialized")
    def on_wasm_application_initialized(self, **kwargs):
        print(
            f"wasm_application_initialized: {self.state.wasm_application_initialized}")

    @change("representation")
    def on_representation_change(self, representation, **kwargs):
        self.call_wasm_application_method("setRepresentation", representation)
        self.update_client_vtk_view()

    @change("vertex_visibility")
    def on_vertex_visibility_change(self, vertex_visibility, **kwargs):
        self.call_wasm_application_method(
            "setVertexVisibility", vertex_visibility)
        self.update_client_vtk_view()

    @change("opacity")
    def on_opacity_change(self, opacity, **kwargs):
        """
        Set opacity. FIXME: Works with VTK master. Needs work in vtkOpenGLLowMemPolyDataMapper
        """
        self.call_wasm_application_method("setOpacity", opacity)
        self.update_client_vtk_view()

    @change("edge_opacity")
    def on_edge_opacity_change(self, edge_opacity, **kwargs):
        self.call_wasm_application_method("setEdgeOpacity", edge_opacity)
        self.update_client_vtk_view()

    @change("point_size")
    def on_point_size_change(self, point_size, **kwargs):
        self.call_wasm_application_method("setPointSize", point_size)
        self.update_client_vtk_view()

    @change("line_width")
    def on_line_width_change(self, line_width, **kwargs):
        self.call_wasm_application_method("setLineWidth", line_width)
        self.update_client_vtk_view()

    @change("color_mode")
    def on_color_mode_change(self, color_mode, **kwargs):
        self.call_wasm_application_method("setColorByArray", color_mode)
        self.update_client_vtk_view()

    @change("color_preset")
    def on_color_preset_change(self, color_preset, **kwargs):
        self.call_wasm_application_method("setColorMapPreset", color_preset)
        self.update_client_vtk_view()

    @change("color_value")
    def on_color_value_change(self, color_value, **kwargs):
        self.call_wasm_application_method("setColor", color_value.get('r'),
                                          color_value.get('g'), color_value.get('b'))
        self.update_client_vtk_view()

    @change("interp_scalars_before_mapping")
    def on_interp_scalars_before_mapping_change(self, interp_scalars_before_mapping, **kwargs):
        self.call_wasm_application_method(
            "setInterpolateScalarsBeforeMapping", interp_scalars_before_mapping)
        self.update_client_vtk_view()

    @change("vertex_color_value")
    def on_vertex_color_value_change(self, vertex_color_value, **kwargs):
        self.call_wasm_application_method("setVertexColor", vertex_color_value.get('r'),
                                          vertex_color_value.get('g'), vertex_color_value.get('b'))
        self.update_client_vtk_view()

    @change("edge_color_value")
    def on_edge_color_value_change(self, edge_color_value, **kwargs):
        self.call_wasm_application_method("setEdgeColor", edge_color_value.get('r'),
                                          edge_color_value.get('g'), edge_color_value.get('b'))
        self.update_client_vtk_view()

    @change("tab_idx")
    def on_tab_change(self, tab_idx, **kwargs):
        """
        Set current tab index and update both the views.
        """
        self.current_editor_tab_idx = tab_idx
        self.update_client_shader_view()

    @change("shader_log")
    def on_shader_log_change(self, *args, **kwargs):
        """
        Renders the client VTK view when shader compilation succeded.
        """
        if self.state.shader_log == "Success!":
            self.update_client_vtk_view()

    def on_editor_input(self, value):
        """
        When developer edits the shader source code, this callback is
        meant to set new shader source code for the VTK shader program.
        """
        if self._block_shader_set:
            return
        setter = SHADER_SET_METHODS[self.current_editor_tab_idx]
        self.call_wasm_application_method_return_value(
            'shader_log', setter, value)

    # UI ----------------------------------------------------------------------

    @hot_reload
    def ui_card(self, title, ui_name, **kwargs):
        with vuetify.VCard(**kwargs):
            vuetify.VCardTitle(
                title,
                classes="grey lighten-1 py-1 grey--text text--darken-3",
                style="user-select: none; cursor: pointer",
                hide_details=True,
                dense=True,
            )
            content = vuetify.VCardText(classes="py-2")
        return content

    @hot_reload
    def mesh_card(self):
        with self.ui_card(title="Mesh", ui_name="mesh_panel"):
            with vuetify.VCol(classes="pa-0 ma-0"):
                # Representation
                vuetify.VSelect(
                    label="Representation",
                    v_model=("representation", Representation.Surface),
                    items=("representations", self.representations),
                )
                # Vertex visibility
                vuetify.VCheckbox(
                    v_model=("vertex_visibility", False),
                    label="Show Vertices",
                    classes="mx-1",
                    hide_details=True,
                    dense=True,
                )
                # Point Size
                vuetify.VSlider(
                    v_model=("point_size", 1.0),
                    min=0,
                    max=10,
                    step=0.1,
                    label="Point Size",
                    classes="mt-1",
                    hide_details=True,
                    dense=True,
                )
                # Line Width
                vuetify.VSlider(
                    v_model=("line_width", 1.0),
                    min=0,
                    max=6,
                    step=0.1,
                    label="Line Width",
                    classes="mt-1",
                    hide_details=True,
                    dense=True,
                )

    @hot_reload
    def color_card(self):
        with self.ui_card(title="Color", ui_name="color_panel"):
            with vuetify.VCol(classes="pa-0 ma-0"):
                with vuetify.VRow():
                    # Color By
                    with vuetify.VCol(cols="6"):
                        vuetify.VSelect(
                            label="Color by",
                            v_model=("color_mode", "Solid"),
                            items=("color_modes", ["Solid"]),
                        )
                    with vuetify.VCol(cols="6"):
                        # Color Map
                        vuetify.VSelect(
                            label="Color map",
                            v_if=("color_mode !== 'Solid'"),
                            v_model=("color_preset", "Spectrum"),
                            items=("color_presets", []),
                        )
                # Interpolate scalars before mapping
                vuetify.VCheckbox(
                    v_model=("interp_scalars_before_mapping", False),
                    label="Interpolate scalars before mapping",
                    classes="mx-1",
                    hide_details=True,
                    dense=True,
                )
                # Opacity
                vuetify.VSlider(
                    v_model=("opacity", 1.0),
                    min=0,
                    max=1,
                    step=0.1,
                    label="Opacity",
                    classes="mt-1",
                    hide_details=True,
                    dense=True,
                )
                # Edge Opacity
                vuetify.VSlider(
                    v_model=("edge_opacity", 1.0),
                    min=0,
                    max=1,
                    step=0.1,
                    label="Edge Opacity",
                    classes="mt-1",
                    hide_details=True,
                    dense=True,
                )
                with vuetify.VRow():
                    # Edge Color Picker
                    with vuetify.VCardText("Edge Color",
                                           v_if=("representation == 3")):
                        vuetify.VColorPicker(
                            mode="rgba",
                            v_model=("edge_color_value", {
                                'r': 255, 'g': 255, 'b': 255, 'a': 1}),
                        )
                    with vuetify.VCardText("Mesh Color",
                                           v_if=("color_mode == 'Solid'")):
                        # Solid Color Picker
                        vuetify.VColorPicker(
                            mode="rgba",
                            v_model=("color_value", {
                                'r': 255, 'g': 255, 'b': 255, 'a': 1}),
                        )
                    # Vertex Color Picker
                    with vuetify.VCardText("Vertex Color"):
                        vuetify.VColorPicker(
                            mode="rgba",
                            v_model=("vertex_color_value", {
                                'r': 255, 'g': 255, 'b': 255, 'a': 1}),
                        )

    @hot_reload
    def ui(self, **kwargs):
        # Consistent defaults match with wasm settings to avoid going out of sync during hot-reload
        self.state.wasm_application_initialized = False
        self.state.shader_log = "Success!"
        self.state.show_shader_log = False
        self.current_editor_tab_idx = 0
        self.state.editor_content = ""
        self.state.color_modes = ["Solid"]
        self.state.files = None
        self.state.representation = Representation.Surface
        self.state.vertex_visibility = False
        self.state.point_size = 1.0
        self.state.line_width = 1.0
        self.state.color_mode = "Solid"
        self.state.color_preset = "Spectrum"
        self.state.color_value = {'r': 255, 'g': 255, 'b': 255, 'a': 1}
        self.state.interp_scalars_before_mapping = False
        self.state.opacity = 1.0
        self.state.edge_opacity = 1.0

        with SinglePageWithDrawerLayout(self.server) as layout:
            layout.title.set_text(self.state.trame__title)
            self.ctrl.call_wasm_application_method = client.JSEval(
                exec="document.querySelector('.js').contentWindow.wasmApplication[$event.method](...$event.args)",
            ).exec
            self.ctrl.call_wasm_application_method_return_value = client.JSEval(
                exec="set($event.var, document.querySelector('.js').contentWindow.wasmApplication[$event.method](...$event.args))",
            ).exec
            self.ctrl.call_js_method = client.JSEval(
                exec="document.querySelector('.js').contentWindow[$event.method](...$event.args)",
            ).exec
            self.ctrl.call_js_method_return_value = client.JSEval(
                exec="set($event.var, document.querySelector('.js').contentWindow.wasmApplication[$event.method](...$event.args))",
            ).exec

            with layout.toolbar as toolbar:
                toolbar.dense = True
                vuetify.VSpacer()
                vuetify.VFileInput(
                    multiple=True,
                    show_size=True,
                    small_chips=True,
                    truncate_length=25,
                    change="document.querySelector('.js').contentWindow.loadFiles($event)",
                    dense=True,
                    hide_details=True,
                    style="max-width: 300px;",
                    accept=".obj, .ply, .vtk, .vtp, .vtu",
                    __properties=["accept"],
                )
                vuetify.VSpacer()
                vuetify.VDivider(classes="mx-3 align-self-center",
                                 vertical=True, inset=True)
                vuetify.VBtn(
                    "Update UI",
                    click=self.ui
                )
                with vuetify.VBtn(icon=True, click=self.reset_camera):
                    vuetify.VIcon("mdi-crop-free")
                vuetify.VCheckbox(
                    v_model=("spin_view", False),
                    on_icon="mdi-rotate-3d",
                    off_icon="mdi-rotate-3d",
                    hide_details=True,
                )
                vuetify.VDivider(classes="mx-3 align-self-center",
                                 vertical=True, inset=True)
                with vuetify.VBtn(icon=True, click=self.update_client_shader_view):
                    vuetify.VIcon("mdi-code-tags")
                with vuetify.VBtn(icon=True, v_if=("shader_log == 'Success!'"), click="show_shader_log = !show_shader_log"):
                    vuetify.VIcon("mdi-code-tags-check", color='success')
                with vuetify.VBtn(icon=True, v_if=("shader_log !== 'Success!'"), click="show_shader_log = !show_shader_log"):
                    vuetify.VIcon("mdi-alert", color='error')
                with vuetify.VDialog(v_model=("show_shader_log", False)):
                    with vuetify.VCard(classes="mx-auto", elevation="16"):
                        vuetify.VTextarea(v_model=("shader_log", ""))
                    vuetify.VBtn("Close", click="show_shader_log = false")
                vuetify.VDivider(classes="mx-3 align-self-center",
                                 vertical=True, inset=True)
                vuetify.VCheckbox(
                    v_model="$vuetify.theme.dark",
                    on_icon="mdi-theme-light-dark",
                    off_icon="mdi-theme-light-dark",
                    hide_details=True,
                )

            # Widgets to change visual properties of the mesh.
            with layout.drawer as drawer:
                self.mesh_card()
                self.color_card()

            # Main content.
            # |              |                  |
            # |  RenderView  |  Shader Editors  |
            # |              |                  |
            with layout.content:
                with vuetify.VContainer(fluid=True, classes="fill-height"):
                    with vuetify.VRow(classes="pa-0 ma-0 fill-height"):
                        # VTK WebAssembly Render View
                        with vuetify.VCol():
                            with vuetify.VCard(classes="pa-0 ma-0 fill-height", loading=("!wasm_application_initialized",)):
                                html.Iframe(src="/wasm/index.html", classes="js fill-height",
                                            style="width: 100%; border: none;")
                        # Shader Editors
                        with vuetify.VCol():
                            with vuetify.VCard(classes="pa-0 ma-0 fill-height"):
                                with vuetify.VTabs(v_model=("tab_idx", 0)):
                                    for tabName, tabValue in SHADER_TYPES:
                                        vuetify.VTab(tabName, value=tabValue)
                                editor = code.Editor(
                                    value=("editor_content", ""),
                                    options=("editor_options", {
                                        "automaticLayout": False}),
                                    language=("editor_lang", "wgsl"),
                                    theme=("editor_theme", "vs-dark"),
                                    textmate=("editor_textmate", None),
                                    input=(self.on_editor_input, "[$event]")
                                )


if __name__ == "__main__":
    d = Driver()
    d.server.start()
