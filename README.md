# Real-Time 3D Rendering Pipeline (C++ / OpenGL)

This project is a self-built real-time rendering pipeline implemented in C++ and OpenGL.

## Features
- Vertex and fragment shader pipeline
- Camera system (view/projection transformations)
- Real-time object transformations
- Interactive controls
- Depth testing and rendering pipeline debugging

## Technical Focus
- Understanding GPU pipeline (vertex -> fragment)
- Shader programming (GLSL)
- Matrix transformations and coordinate systems
- Debugging rendering artifacts and pipeline issues

## Next Steps
- Lighting system (Phong / PBR)
- Shadow mapping
- Deferred rendering pipeline

## Build And Run

This project provides a minimal C++ + OpenGL runner that can load Shadertoy-style fragment shaders (with `mainImage(out vec4, in vec2)`) and display them in a window.

Build (macOS):

```bash
# install cmake if needed
brew install cmake

mkdir build && cd build
cmake ..
cmake --build .

# run (optional path to fragment shader)
./shader_app ../Shadertoy_Fragment/morph_sphere_cube.frag
```

- Notes:
- The CMakeLists uses FetchContent to download `glfw` at configure time and uses the system OpenGL headers on macOS (no network download for GL loader).
- The runner wraps Shadertoy-style fragment shaders by prepending a small compatibility header and calling `mainImage` from `main()`.
- If your fragment shader contains its own `main()`, you'll need to adapt it to `mainImage` or edit the code.
