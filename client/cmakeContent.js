export const cmakeContent =
    'cmake_minimum_required(VERSION 3.13)\n' +
    'project(main)\n' +
    '\n' +
    '# -----------------------------------------------------------------------------\n' +
    '# EMSCRIPTEN only\n' +
    '# -----------------------------------------------------------------------------\n' +
    '\n' +
    'if (NOT EMSCRIPTEN)\n' +
    '  message("Skipping example: This needs to run inside an Emscripten build environment")\n' +
    '  return ()\n' +
    'endif ()\n' +
    '\n' +
    '# -----------------------------------------------------------------------------\n' +
    '# Handle VTK dependency\n' +
    '# -----------------------------------------------------------------------------\n' +
    '\n' +
    'find_package(VTK)\n' +
    '\n' +
    'if (NOT VTK_FOUND)\n' +
    '  message("Skipping example: ${VTK_NOT_FOUND_MESSAGE}")\n' +
    '  return ()\n' +
    'endif ()\n' +
    '\n' +
    '# -----------------------------------------------------------------------------\n' +
    '# Compile example code\n' +
    '# -----------------------------------------------------------------------------\n' +
    '\n' +
    'add_executable(main main.cpp)\n' +
    'target_link_libraries(main PRIVATE ${VTK_LIBRARIES})\n' +
    '\n' +
    '# -----------------------------------------------------------------------------\n' +
    '# Optimizations\n' +
    '# -----------------------------------------------------------------------------\n' +
    '\n' +
    'set(emscripten_optimizations)\n' +
    'set(emscripten_debug_options)\n' +
    '\n' +
    '# Set a default build type if none was specified\n' +
    'if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)\n' +
    '  message(STATUS "Setting build type to "Debug" as none was specified.")\n' +
    '  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)\n' +
    '  # Set the possible values of build type for cmake-gui\n' +
    '  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"\n' +
    '    "MinSizeRel" "RelWithDebInfo")\n' +
    'endif()\n' +
    '\n' +
    'if (CMAKE_BUILD_TYPE STREQUAL "Release")\n' +
    '  set(main_wasm_optimize "BEST")\n' +
    '  set(main_wasm_debuginfo "NONE")\n' +
    'elseif (CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")\n' +
    '  set(main_wasm_optimize "SMALLEST_WITH_CLOSURE")\n' +
    '  set(main_wasm_debuginfo "NONE")\n' +
    'elseif (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")\n' +
    '  set(main_wasm_optimize "MORE")\n' +
    '  set(main_wasm_debuginfo "PROFILE")\n' +
    'elseif (CMAKE_BUILD_TYPE STREQUAL "Debug")\n' +
    '  set(main_wasm_optimize "NO_OPTIMIZATION")\n' +
    '  set(main_wasm_debuginfo "DEBUG_NATIVE")\n' +
    'endif ()\n' +
    'set(main_wasm_optimize_NO_OPTIMIZATION "-O0")\n' +
    'set(main_wasm_optimize_LITTLE "-O1")\n' +
    'set(main_wasm_optimize_MORE "-O2")\n' +
    'set(main_wasm_optimize_BEST "-O3")\n' +
    'set(main_wasm_optimize_SMALLEST "-Os")\n' +
    'set(main_wasm_optimize_SMALLEST_WITH_CLOSURE "-Oz")\n' +
    'set(main_wasm_optimize_SMALLEST_WITH_CLOSURE_link "--closure=1")\n' +
    '\n' +
    'if (DEFINED "main_wasm_optimize_${main_wasm_optimize}")\n' +
    '  list(APPEND emscripten_optimizations\n' +
    '    ${main_wasm_optimize_${main_wasm_optimize}})\n' +
    '  list(APPEND emscripten_link_options\n' +
    '    ${main_wasm_optimize_${main_wasm_optimize}_link})\n' +
    'else ()\n' +
    '  message (FATAL_ERROR "Unrecognized value for main_wasm_optimize=${main_wasm_optimize}")\n' +
    'endif ()\n' +
    '\n' +
    'set(main_wasm_debuginfo_NONE "-g0")\n' +
    'set(main_wasm_debuginfo_READABLE_JS "-g1")\n' +
    'set(main_wasm_debuginfo_PROFILE "-g2")\n' +
    'set(main_wasm_debuginfo_DEBUG_NATIVE "-g3")\n' +
    'set(main_wasm_debuginfo_DEBUG_NATIVE_link "-sASSERTIONS=1")\n' +
    'if (DEFINED "main_wasm_debuginfo_${main_wasm_debuginfo}")\n' +
    '  list(APPEND emscripten_debug_options\n' +
    '    ${main_wasm_debuginfo_${main_wasm_debuginfo}})\n' +
    '  list(APPEND emscripten_link_options\n' +
    '    ${main_wasm_debuginfo_${main_wasm_debuginfo}_link})\n' +
    'else ()\n' +
    '  message (FATAL_ERROR "Unrecognized value for main_wasm_debuginfo=${main_wasm_debuginfo}")\n' +
    'endif ()\n' +
    '\n' +
    'target_compile_options(main\n' +
    '  PRIVATE\n' +
    '    ${emscripten_compile_options}\n' +
    '    ${emscripten_optimizations}\n' +
    '    ${emscripten_debug_options})\n' +
    'target_link_options(main\n' +
    '  PRIVATE\n' +
    '    ${emscripten_link_options}\n' +
    '    ${emscripten_optimizations}\n' +
    '    ${emscripten_debug_options}\n' +
    '    "-sSINGLE_FILE=1"\n' +
    '    "--shell-file=${CMAKE_CURRENT_LIST_DIR}/shell.html")\n' +
    '\n' +
    'set_target_properties(main\n' +
    '  PROPERTIES\n' +
    '    SUFFIX ".html"\n' +
    ')\n' +
    '# -----------------------------------------------------------------------------\n' +
    '# VTK modules initialization\n' +
    '# -----------------------------------------------------------------------------\n' +
    '\n' +
    'vtk_module_autoinit(\n' +
    '  TARGETS  main\n' +
    '  MODULES  ${VTK_LIBRARIES}\n' +
    ')\n'
    ;