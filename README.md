# 3D Sierpinski Tetrahedron Ray Marching

A real-time 3D fractal renderer that visualizes a rotating Sierpinski tetrahedron using GPU-accelerated ray marching in OpenGL/GLSL.

![Sierpinski Tetrahedron](https://img.shields.io/badge/OpenGL-3.3+-blue) ![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey) ![Language](https://img.shields.io/badge/Language-C-brightgreen)

## Features

- **GPU Ray Marching**: Entire fractal rendering performed in fragment shader for maximum performance
- **Sierpinski Tetrahedron**: Iterative folding algorithm creates intricate 3D fractal geometry
- **Dynamic Lighting**: Lambertian diffuse + Blinn-Phong specular with ambient occlusion
- **Animated Colors**: Hue shifts based on iteration depth and time
- **Smooth Rotation**: Automatic Y-axis rotation using time uniform
- **Optimized SDF**: Fast signed distance estimator with 12 iterations
- **Fog/Depth Attenuation**: Atmospheric depth cueing for better spatial perception

## Requirements

### Runtime Dependencies
- **SDL2**: Window management and OpenGL context creation
- **GLEW**: OpenGL extension loading
- **OpenGL 3.3+**: Core profile support

### Build Tools
- **MinGW-w64** (gcc) or **MSVC** compiler
- Windows 7 or later

## Installation

### 1. Install Dependencies (Windows)

#### Option A: MSYS2 (Recommended)
```bash
# Install MSYS2 from https://www.msys2.org/
# Open MSYS2 MinGW 64-bit terminal

pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-SDL2 mingw-w64-x86_64-glew
```

#### Option B: Manual Installation
1. Download SDL2 development libraries (MinGW): https://www.libsdl.org/download-2.0.php
2. Download GLEW binaries: http://glew.sourceforge.net/
3. Extract and place libraries in your MinGW `lib` and `include` directories

### 2. Compile the Application

```bash
gcc -o sierpinski.exe sierpinski.c -lSDL2main -lSDL2 -lglew32 -lopengl32 -lm
```

### 3. Run

```bash
./sierpinski.exe
```

Or double-click `sierpinski.exe` in Windows Explorer.

## Usage

- **ESC**: Exit application
- **Close Window**: Terminate program

The fractal automatically rotates. No user interaction required for animation.

## Project Structure

```
.
├── sierpinski.c        # Main C application (includes embedded shaders)
├── shader.vert         # Vertex shader (for reference, embedded in .c)
├── shader.frag         # Fragment shader (for reference, embedded in .c)
└── README.md           # This file
```

## Technical Details

### Ray Marching Algorithm
The fragment shader implements sphere tracing:
1. Cast ray from camera through each pixel
2. Iteratively march along ray using SDF distance estimates
3. Stop when distance < epsilon (surface hit) or max distance exceeded
4. Compute lighting and color at intersection point

### Sierpinski SDF
The signed distance function uses iterative tetrahedral folding:
```glsl
for each iteration:
    1. Find closest tetrahedron vertex (a1, a2, a3, a4)
    2. Scale and fold space: p = scale * p - c * (scale - 1.0)
    3. Track iteration count for coloring
```

### Performance Optimizations
- **Max 256 march steps**: Prevents infinite loops
- **Relaxation factor 0.5**: Conservative stepping for stability
- **Early exit**: Stops marching on surface hit
- **Epsilon threshold 0.001**: Balance between quality and speed
- **12 SDF iterations**: Optimal detail vs. computation trade-off

### Lighting Model
- **Diffuse**: Lambertian with dynamic light direction
- **Specular**: Blinn-Phong (exponent 32) for highlights
- **Ambient Occlusion**: 5-sample approximation for depth shadowing
- **Fog**: Exponential depth attenuation
- **Gamma Correction**: sRGB output (power 0.4545)

## Customization

### Adjust Fractal Complexity
In `sierpinskiSDF()` function:
```glsl
const int iterations = 12;  // Increase for more detail (slower)
const float scale = 2.0;    // Modify for different patterns
```

### Change Rotation Speed
In `main()` shader function:
```glsl
mat3 rot = rotateY(u_time * 0.3);  // Multiply by desired speed
```

### Modify Colors
In color calculation block:
```glsl
vec3 baseColor = vec3(
    0.5 + 0.5 * sin(stepRatio * 6.28 + u_time),        // Red channel
    0.5 + 0.5 * sin(stepRatio * 6.28 + u_time + 2.09), // Green channel
    0.5 + 0.5 * sin(stepRatio * 6.28 + u_time + 4.18)  // Blue channel
);
```

### Window Resolution
In `sierpinski.c` `main()`:
```c
SDL_Window* window = SDL_CreateWindow(
    "Sierpinski Tetrahedron - Ray Marching",
    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
    1280, 720,  // Change width/height here
    SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN
);
```

## Troubleshooting

### "SDL2.dll not found"
- Ensure SDL2.dll is in the same directory as sierpinski.exe
- Or add SDL2/bin directory to your system PATH

### "Failed to create OpenGL context"
- Update graphics drivers
- Your GPU may not support OpenGL 3.3 (try changing version in code)

### Black screen / No rendering
- Check console for shader compilation errors
- Verify GLEW initialization succeeded
- Ensure GPU supports required OpenGL extensions

### Poor performance
- Reduce window resolution
- Decrease `iterations` in Sierpinski SDF
- Lower `maxSteps` in ray marching loop
- Disable ambient occlusion (comment out `calcAO`)

## Building for MSVC

If using Visual Studio instead of MinGW:

```bash
cl sierpinski.c /I"C:\path\to\SDL2\include" /I"C:\path\to\GLEW\include" ^
   /link /LIBPATH:"C:\path\to\SDL2\lib" /LIBPATH:"C:\path\to\GLEW\lib" ^
   SDL2.lib SDL2main.lib glew32.lib opengl32.lib /SUBSYSTEM:CONSOLE
```

## License

This code is provided as-is for educational and demonstration purposes. Free to use, modify, and distribute.

## References

- **Ray Marching**: [Inigo Quilez - Distance Functions](https://iquilezles.org/articles/distfunctions/)
- **Sierpinski Fractals**: [Wikipedia - Sierpiński Triangle](https://en.wikipedia.org/wiki/Sierpi%C5%84ski_triangle)
- **SDL2 Documentation**: https://wiki.libsdl.org/
- **OpenGL Reference**: https://www.khronos.org/opengl/

## Author

- Twitter: [@positronx_](https://twitter.com/positronx_)

## Support My Work

[![Buy Me a Coffee](https://www.buymeacoffee.com/assets/img/custom_images/orange_img.png)](http://buymeacoffee.com/positronx_)

---

**Enjoy exploring the infinite depth of the Sierpinski tetrahedron!** 🔺✨
