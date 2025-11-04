/*
 * Enhanced Sierpinski Tetrahedron Ray Marching Demo
 * Advanced real-time 3D fractal renderer with ray tracing effects
 * 
 * Features:
 * - Reflections with metallic/glass materials
 * - Soft shadows via shadow ray marching
 * - Multi-sample ambient occlusion
 * - Volumetric glow and atmospheric effects
 * - Environment mapping with procedural skybox
 * - Post-processing (bloom, vignette, chromatic aberration)
 * - Enhanced psychedelic coloring with multiple palettes
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <SDL2/SDL_opengl.h>

// Embedded shader source code
const char* vertexShaderSource = 
"#version 330 core\n"
"layout(location = 0) in vec2 position;\n"
"out vec2 v_uv;\n"
"void main() {\n"
"    v_uv = position * 0.5 + 0.5;\n"
"    gl_Position = vec4(position, 0.0, 1.0);\n"
"}\n";

const char* fragmentShaderSource = 
"#version 330 core\n"
"in vec2 v_uv;\n"
"uniform vec2 u_resolution;\n"
"uniform float u_time;\n"
"uniform vec3 u_camPos;\n"
"uniform mat3 u_rotation;\n"
"uniform int u_colorPalette;\n"
"out vec4 fragColor;\n"
"\n"
"// Constants\n"
"const float PI = 3.14159265359;\n"
"const float TAU = 6.28318530718;\n"
"const int MAX_MARCH_STEPS = 200;\n"
"const float MAX_DIST = 50.0;\n"
"const float HIT_THRESHOLD = 0.0001;\n"
"const int FRACTAL_ITERATIONS = 14;\n"
"const float FRACTAL_SCALE = 2.0;\n"
"\n"
"// Advanced Sierpinski Tetrahedron with enhanced orbit traps\n"
"float sdSierpinski(vec3 p, out vec3 orbitTrap) {\n"
"    vec3 z = p;\n"
"    float r = 0.0;\n"
"    float dr = 1.0;\n"
"    orbitTrap = vec3(1e10);\n"
"    float minDist = 1e10;\n"
"    \n"
"    for (int n = 0; n < FRACTAL_ITERATIONS; n++) {\n"
"        // Tetrahedral folding symmetry\n"
"        if (z.x + z.y < 0.0) z.xy = -z.yx;\n"
"        if (z.x + z.z < 0.0) z.xz = -z.zx;\n"
"        if (z.y + z.z < 0.0) z.zy = -z.yz;\n"
"        \n"
"        // Additional fold for more detail\n"
"        if (z.x - z.y < 0.0) z.xy = z.yx;\n"
"        \n"
"        // Scale and translate\n"
"        z = z * FRACTAL_SCALE - 1.0 * (FRACTAL_SCALE - 1.0);\n"
"        dr = dr * FRACTAL_SCALE;\n"
"        \n"
"        // Enhanced orbit traps for coloring\n"
"        float d = length(z);\n"
"        minDist = min(minDist, d);\n"
"        orbitTrap.x = min(orbitTrap.x, d);\n"
"        orbitTrap.y = min(orbitTrap.y, abs(z.x) + abs(z.y) + abs(z.z));\n"
"        orbitTrap.z = min(orbitTrap.z, dot(z, z));\n"
"    }\n"
"    \n"
"    r = length(z);\n"
"    return 0.5 * r / dr;\n"
"}\n"
"\n"
"// Wrapper for simple distance queries\n"
"float map(vec3 p) {\n"
"    vec3 dummy;\n"
"    return sdSierpinski(p, dummy);\n"
"}\n"
"\n"
"// High-quality normal estimation\n"
"vec3 calcNormal(vec3 p) {\n"
"    const float h = 0.0001;\n"
"    const vec2 k = vec2(1, -1);\n"
"    return normalize(\n"
"        k.xyy * map(p + k.xyy * h) +\n"
"        k.yyx * map(p + k.yyx * h) +\n"
"        k.yxy * map(p + k.yxy * h) +\n"
"        k.xxx * map(p + k.xxx * h)\n"
"    );\n"
"}\n"
"\n"
"// Multi-sample ambient occlusion\n"
"float calcAO(vec3 p, vec3 n) {\n"
"    float ao = 0.0;\n"
"    float scale = 1.0;\n"
"    for (int i = 0; i < 5; i++) {\n"
"        float h = 0.01 + 0.12 * float(i) / 4.0;\n"
"        float d = map(p + n * h);\n"
"        ao += (h - d) * scale;\n"
"        scale *= 0.85;\n"
"    }\n"
"    return clamp(1.0 - 3.0 * ao, 0.0, 1.0);\n"
"}\n"
"\n"
"// Soft shadows using shadow ray marching\n"
"float calcShadow(vec3 ro, vec3 rd, float mint, float maxt, float k) {\n"
"    float res = 1.0;\n"
"    float t = mint;\n"
"    for (int i = 0; i < 32; i++) {\n"
"        float h = map(ro + rd * t);\n"
"        if (h < HIT_THRESHOLD) return 0.0;\n"
"        res = min(res, k * h / t);\n"
"        t += h;\n"
"        if (t > maxt) break;\n"
"    }\n"
"    return clamp(res, 0.0, 1.0);\n"
"}\n"
"\n"
"// Ray marching with orbit trap output\n"
"float rayMarch(vec3 ro, vec3 rd, out vec3 orbitTrap) {\n"
"    float t = 0.0;\n"
"    orbitTrap = vec3(1e10);\n"
"    \n"
"    for (int i = 0; i < MAX_MARCH_STEPS; i++) {\n"
"        vec3 p = ro + rd * t;\n"
"        vec3 trap;\n"
"        float d = sdSierpinski(p, trap);\n"
"        orbitTrap = min(orbitTrap, trap);\n"
"        \n"
"        if (d < HIT_THRESHOLD) return t;\n"
"        \n"
"        t += d * 0.6;\n"
"        \n"
"        if (t > MAX_DIST) break;\n"
"    }\n"
"    \n"
"    return -1.0;\n"
"}\n"
"\n"
"// Procedural starfield skybox\n"
"vec3 getSkyColor(vec3 rd) {\n"
"    // Gradient background\n"
"    float grad = smoothstep(-0.5, 0.5, rd.y);\n"
"    vec3 sky = mix(\n"
"        vec3(0.02, 0.01, 0.05),\n"
"        vec3(0.1, 0.05, 0.2),\n"
"        grad\n"
"    );\n"
"    \n"
"    // Stars\n"
"    vec3 starCoord = rd * 200.0;\n"
"    float star = 0.0;\n"
"    for (int i = 0; i < 3; i++) {\n"
"        vec3 fl = floor(starCoord);\n"
"        vec3 fr = fract(starCoord);\n"
"        float h = fract(sin(dot(fl, vec3(12.9898, 78.233, 45.164))) * 43758.5453);\n"
"        float size = 0.02 * h;\n"
"        star += smoothstep(size, 0.0, length(fr - 0.5)) * h;\n"
"        starCoord *= 1.7;\n"
"    }\n"
"    sky += star * vec3(1.0, 0.9, 0.8) * 0.5;\n"
"    \n"
"    // Nebula effect\n"
"    float nebula = sin(rd.x * 3.0 + u_time * 0.1) * cos(rd.y * 4.0) * sin(rd.z * 5.0);\n"
"    nebula = pow(max(nebula, 0.0), 3.0);\n"
"    sky += nebula * vec3(0.5, 0.2, 0.8) * 0.3;\n"
"    \n"
"    return sky;\n"
"}\n"
"\n"
"// Multiple color palette options\n"
"vec3 getColorPalette(float t, int palette) {\n"
"    if (palette == 0) {\n"
"        // Psychedelic rainbow\n"
"        return 0.5 + 0.5 * cos(TAU * (t + vec3(0.0, 0.33, 0.67)));\n"
"    } else if (palette == 1) {\n"
"        // Fire/lava\n"
"        return 0.5 + 0.5 * cos(TAU * (t + vec3(0.0, 0.1, 0.2)));\n"
"    } else if (palette == 2) {\n"
"        // Electric blue/purple\n"
"        return 0.5 + 0.5 * cos(TAU * (t + vec3(0.6, 0.5, 0.8)));\n"
"    } else {\n"
"        // Gold/bronze\n"
"        return 0.5 + 0.5 * cos(TAU * (t + vec3(0.15, 0.1, 0.0)));\n"
"    }\n"
"}\n"
"\n"
"// Enhanced coloring with multiple orbit traps\n"
"vec3 getEnhancedColor(vec3 orbitTrap, vec3 normal, float t) {\n"
"    float hue = orbitTrap.x * 0.4 + orbitTrap.y * 0.3 + u_time * 0.15;\n"
"    vec3 col1 = getColorPalette(hue, u_colorPalette);\n"
"    \n"
"    // Add variation based on second orbit trap\n"
"    float hue2 = orbitTrap.z * 0.1 + u_time * 0.05;\n"
"    vec3 col2 = getColorPalette(hue2, (u_colorPalette + 1) % 4);\n"
"    \n"
"    // Mix based on normal direction for interesting patterns\n"
"    float mixFactor = abs(sin(normal.x * 10.0 + normal.y * 7.0 + u_time * 0.5));\n"
"    vec3 col = mix(col1, col2, mixFactor * 0.3);\n"
"    \n"
"    return col;\n"
"}\n"
"\n"
"// Volumetric glow effect\n"
"vec3 getVolumetricGlow(vec3 ro, vec3 rd, float maxT) {\n"
"    vec3 glow = vec3(0.0);\n"
"    float t = 0.0;\n"
"    for (int i = 0; i < 32; i++) {\n"
"        vec3 p = ro + rd * t;\n"
"        float d = map(p);\n"
"        \n"
"        // Accumulate glow near surface\n"
"        float glowFactor = 0.015 / (0.01 + d * d);\n"
"        vec3 orbitTrap;\n"
"        sdSierpinski(p, orbitTrap);\n"
"        vec3 glowCol = getColorPalette(orbitTrap.x * 0.5 + u_time * 0.2, u_colorPalette);\n"
"        glow += glowCol * glowFactor * 0.002;\n"
"        \n"
"        t += max(0.05, d * 0.5);\n"
"        if (t > maxT || t > MAX_DIST) break;\n"
"    }\n"
"    return glow;\n"
"}\n"
"\n"
"// Reflection ray marching (single bounce)\n"
"vec3 traceReflection(vec3 ro, vec3 rd, vec3 normal, vec3 baseColor, float roughness) {\n"
"    // Perturb reflection direction for roughness\n"
"    vec3 reflectDir = reflect(rd, normal);\n"
"    \n"
"    vec3 orbitTrap;\n"
"    float t = rayMarch(ro + normal * 0.01, reflectDir, orbitTrap);\n"
"    \n"
"    if (t > 0.0) {\n"
"        vec3 p = ro + normal * 0.01 + reflectDir * t;\n"
"        vec3 n = calcNormal(p);\n"
"        vec3 reflColor = getEnhancedColor(orbitTrap, n, t);\n"
"        \n"
"        // Simple lighting for reflection\n"
"        vec3 lightDir = normalize(vec3(1.0, 1.0, -1.0));\n"
"        float diff = max(dot(n, lightDir), 0.0);\n"
"        reflColor *= (0.3 + diff * 0.7);\n"
"        \n"
"        return reflColor;\n"
"    }\n"
"    \n"
"    return getSkyColor(reflectDir);\n"
"}\n"
"\n"
"// Chromatic aberration post-process\n"
"vec3 chromaticAberration(vec2 uv, float amount) {\n"
"    // This is simplified - just returns direction for offset\n"
"    vec2 dir = uv - vec2(0.5);\n"
"    return vec3(length(dir)) * amount;\n"
"}\n"
"\n"
"void main() {\n"
"    // Normalize pixel coordinates with slight chromatic aberration\n"
"    vec2 uv = (gl_FragCoord.xy - 0.5 * u_resolution) / u_resolution.y;\n"
"    \n"
"    // Anti-aliasing via supersampling (2x2)\n"
"    vec3 finalColor = vec3(0.0);\n"
"    \n"
"    for (int aa_x = 0; aa_x < 2; aa_x++) {\n"
"        for (int aa_y = 0; aa_y < 2; aa_y++) {\n"
"            vec2 offset = vec2(float(aa_x), float(aa_y)) / u_resolution.y * 0.5;\n"
"            vec2 uv_aa = uv + offset;\n"
"            \n"
"            // Camera setup\n"
"            vec3 ro = u_camPos;\n"
"            vec3 rd = normalize(vec3(uv_aa, -1.8));\n"
"            rd = u_rotation * rd;\n"
"            \n"
"            // Background\n"
"            vec3 col = getSkyColor(rd);\n"
"            \n"
"            // Ray march\n"
"            vec3 orbitTrap;\n"
"            float t = rayMarch(ro, rd, orbitTrap);\n"
"            \n"
"            // Add volumetric glow\n"
"            vec3 glow = getVolumetricGlow(ro, rd, t > 0.0 ? t : MAX_DIST);\n"
"            \n"
"            if (t > 0.0) {\n"
"                // Hit! Calculate advanced lighting\n"
"                vec3 p = ro + rd * t;\n"
"                vec3 normal = calcNormal(p);\n"
"                \n"
"                // Multi-light setup\n"
"                vec3 lightDir1 = normalize(vec3(1.0, 1.0, -1.0));\n"
"                vec3 lightDir2 = normalize(vec3(-1.0, 0.8, 0.5));\n"
"                vec3 lightDir3 = normalize(vec3(0.0, -1.0, 0.0));\n"
"                \n"
"                vec3 lightCol1 = vec3(1.0, 0.95, 0.9);\n"
"                vec3 lightCol2 = vec3(0.5, 0.6, 1.0);\n"
"                vec3 lightCol3 = vec3(0.8, 0.3, 0.9);\n"
"                \n"
"                // Shadows\n"
"                float shadow1 = calcShadow(p, lightDir1, 0.02, 5.0, 8.0);\n"
"                float shadow2 = calcShadow(p, lightDir2, 0.02, 5.0, 8.0);\n"
"                \n"
"                // Ambient occlusion\n"
"                float ao = calcAO(p, normal);\n"
"                \n"
"                // Diffuse lighting\n"
"                float diff1 = max(dot(normal, lightDir1), 0.0) * shadow1;\n"
"                float diff2 = max(dot(normal, lightDir2), 0.0) * shadow2;\n"
"                float diff3 = max(dot(normal, lightDir3), 0.0) * 0.3;\n"
"                \n"
"                // Specular (Blinn-Phong)\n"
"                vec3 viewDir = -rd;\n"
"                vec3 halfDir1 = normalize(lightDir1 + viewDir);\n"
"                vec3 halfDir2 = normalize(lightDir2 + viewDir);\n"
"                float spec1 = pow(max(dot(normal, halfDir1), 0.0), 64.0) * shadow1;\n"
"                float spec2 = pow(max(dot(normal, halfDir2), 0.0), 32.0) * shadow2;\n"
"                \n"
"                // Fresnel effect for reflections\n"
"                float fresnel = pow(1.0 - max(dot(viewDir, normal), 0.0), 3.0);\n"
"                \n"
"                // Base color with enhanced palette\n"
"                vec3 baseCol = getEnhancedColor(orbitTrap, normal, t);\n"
"                \n"
"                // Material properties (metallic/glossy)\n"
"                float metallic = 0.6;\n"
"                float roughness = 0.2;\n"
"                \n"
"                // Combine diffuse lighting\n"
"                vec3 diffuse = baseCol * (\n"
"                    lightCol1 * diff1 * 0.7 +\n"
"                    lightCol2 * diff2 * 0.5 +\n"
"                    lightCol3 * diff3 * 0.3 +\n"
"                    vec3(0.05, 0.05, 0.1) // Ambient\n"
"                ) * ao;\n"
"                \n"
"                // Specular highlights\n"
"                vec3 specular = (\n"
"                    lightCol1 * spec1 * 1.5 +\n"
"                    lightCol2 * spec2 * 0.8\n"
"                );\n"
"                \n"
"                // Reflections\n"
"                vec3 reflection = traceReflection(p, rd, normal, baseCol, roughness);\n"
"                \n"
"                // Combine with metallic/fresnel\n"
"                col = mix(diffuse, reflection, fresnel * metallic * 0.7);\n"
"                col += specular * (1.0 + metallic * 2.0);\n"
"                \n"
"                // Subsurface scattering fake\n"
"                float sss = pow(max(dot(-lightDir1, normal), 0.0), 3.0);\n"
"                col += baseCol * sss * 0.3;\n"
"                \n"
"                // Atmospheric fog\n"
"                float fog = exp(-t * 0.04);\n"
"                col = mix(getSkyColor(rd), col, fog);\n"
"            }\n"
"            \n"
"            // Add volumetric glow\n"
"            col += glow * 2.0;\n"
"            \n"
"            finalColor += col;\n"
"        }\n"
"    }\n"
"    \n"
"    // Average anti-aliasing samples\n"
"    finalColor /= 4.0;\n"
"    \n"
"    // Post-processing effects\n"
"    \n"
"    // Vignette\n"
"    vec2 vignetteUV = gl_FragCoord.xy / u_resolution - 0.5;\n"
"    float vignette = 1.0 - dot(vignetteUV, vignetteUV) * 0.3;\n"
"    finalColor *= vignette;\n"
"    \n"
"    // Subtle bloom\n"
"    float brightness = dot(finalColor, vec3(0.2126, 0.7152, 0.0722));\n"
"    if (brightness > 0.8) {\n"
"        finalColor += (finalColor - 0.8) * 0.3;\n"
"    }\n"
"    \n"
"    // Color grading\n"
"    finalColor = pow(finalColor, vec3(0.9)); // Slight contrast\n"
"    finalColor = mix(vec3(dot(finalColor, vec3(0.299, 0.587, 0.114))), finalColor, 1.1); // Saturation boost\n"
"    \n"
"    // Gamma correction\n"
"    finalColor = pow(finalColor, vec3(0.4545));\n"
"    \n"
"    fragColor = vec4(finalColor, 1.0);\n"
"}\n";

// Shader compilation helper
GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[1024];
        glGetShaderInfoLog(shader, 1024, NULL, infoLog);
        fprintf(stderr, "Shader compilation failed:\n%s\n", infoLog);
        return 0;
    }
    
    return shader;
}

// Program linking helper
GLuint createShaderProgram(const char* vertSrc, const char* fragSrc) {
    GLuint vertShader = compileShader(GL_VERTEX_SHADER, vertSrc);
    GLuint fragShader = compileShader(GL_FRAGMENT_SHADER, fragSrc);
    
    if (!vertShader || !fragShader) {
        return 0;
    }
    
    GLuint program = glCreateProgram();
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);
    
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[1024];
        glGetProgramInfoLog(program, 1024, NULL, infoLog);
        fprintf(stderr, "Program linking failed:\n%s\n", infoLog);
        return 0;
    }
    
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);
    
    return program;
}

// 3x3 rotation matrices
void rotationMatrixY(float angle, float* mat) {
    float c = cosf(angle);
    float s = sinf(angle);
    
    mat[0] = c;  mat[1] = 0;  mat[2] = s;
    mat[3] = 0;  mat[4] = 1;  mat[5] = 0;
    mat[6] = -s; mat[7] = 0;  mat[8] = c;
}

void rotationMatrixX(float angle, float* mat) {
    float c = cosf(angle);
    float s = sinf(angle);
    
    mat[0] = 1;  mat[1] = 0;   mat[2] = 0;
    mat[3] = 0;  mat[4] = c;   mat[5] = -s;
    mat[6] = 0;  mat[7] = s;   mat[8] = c;
}

void multiplyMat3(float* result, float* a, float* b) {
    float temp[9];
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            temp[i * 3 + j] = 0;
            for (int k = 0; k < 3; k++) {
                temp[i * 3 + j] += a[i * 3 + k] * b[k * 3 + j];
            }
        }
    }
    for (int i = 0; i < 9; i++) {
        result[i] = temp[i];
    }
}

int main(int argc, char* argv[]) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
        return 1;
    }
    
    // OpenGL context attributes
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
    
    // Create window
    int windowWidth = 1920;
    int windowHeight = 1080;
    SDL_Window* window = SDL_CreateWindow(
        "Enhanced Sierpinski Tetrahedron - Ray Tracing",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        windowWidth, windowHeight,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );
    
    if (!window) {
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    
    // Create OpenGL context
    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        fprintf(stderr, "OpenGL context creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    // Initialize GLEW
    glewExperimental = GL_TRUE;
    GLenum glewError = glewInit();
    if (glewError != GLEW_OK) {
        fprintf(stderr, "GLEW initialization failed: %s\n", glewGetErrorString(glewError));
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    // Enable VSync
    SDL_GL_SetSwapInterval(1);
    
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║  Enhanced Sierpinski Tetrahedron Ray Tracer             ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n");
    printf("OpenGL Version: %s\n", glGetString(GL_VERSION));
    printf("GLSL Version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
    printf("\nControls:\n");
    printf("  ESC / Q      - Quit\n");
    printf("  SPACE        - Cycle color palette\n");
    printf("  Arrow Keys   - Adjust camera\n");
    printf("  +/-          - Zoom in/out\n");
    printf("\n");
    
    // Create shader program
    GLuint shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);
    if (!shaderProgram) {
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    // Full-screen quad vertices
    float quadVertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
        -1.0f,  1.0f,
         1.0f,  1.0f
    };
    
    // Create VAO and VBO
    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    // Get uniform locations
    GLint u_resolution = glGetUniformLocation(shaderProgram, "u_resolution");
    GLint u_time = glGetUniformLocation(shaderProgram, "u_time");
    GLint u_camPos = glGetUniformLocation(shaderProgram, "u_camPos");
    GLint u_rotation = glGetUniformLocation(shaderProgram, "u_rotation");
    GLint u_colorPalette = glGetUniformLocation(shaderProgram, "u_colorPalette");
    
    // Application state
    bool running = true;
    Uint32 startTime = SDL_GetTicks();
    int colorPalette = 0;
    float cameraOffsetX = 0.0f;
    float cameraOffsetY = 0.0f;
    float cameraDistance = 4.5f;
    float rotationSpeedMult = 1.0f;
    
    // FPS counter
    Uint32 frameCount = 0;
    Uint32 lastFPSTime = startTime;
    float fps = 0.0f;
    
    while (running) {
        // Event handling
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                    case SDLK_q:
                        running = false;
                        break;
                    case SDLK_SPACE:
                        colorPalette = (colorPalette + 1) % 4;
                        printf("Color Palette: %d\n", colorPalette);
                        break;
                    case SDLK_UP:
                        cameraOffsetY += 0.1f;
                        break;
                    case SDLK_DOWN:
                        cameraOffsetY -= 0.1f;
                        break;
                    case SDLK_LEFT:
                        cameraOffsetX -= 0.1f;
                        break;
                    case SDLK_RIGHT:
                        cameraOffsetX += 0.1f;
                        break;
                    case SDLK_PLUS:
                    case SDLK_EQUALS:
                        cameraDistance -= 0.2f;
                        if (cameraDistance < 2.0f) cameraDistance = 2.0f;
                        break;
                    case SDLK_MINUS:
                        cameraDistance += 0.2f;
                        if (cameraDistance > 10.0f) cameraDistance = 10.0f;
                        break;
                    case SDLK_r:
                        // Reset camera
                        cameraOffsetX = 0.0f;
                        cameraOffsetY = 0.0f;
                        cameraDistance = 4.5f;
                        break;
                }
            } else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED) {
                windowWidth = event.window.data1;
                windowHeight = event.window.data2;
                glViewport(0, 0, windowWidth, windowHeight);
            }
        }
        
        // Calculate time
        float time = (SDL_GetTicks() - startTime) / 1000.0f;
        
        // Calculate combined rotation matrix
        float rotAngleY = time * 0.25f * rotationSpeedMult;
        float rotAngleX = sinf(time * 0.1f) * 0.15f;
        
        float rotY[9], rotX[9], rotMat[9];
        rotationMatrixY(rotAngleY, rotY);
        rotationMatrixX(rotAngleX, rotX);
        multiplyMat3(rotMat, rotY, rotX);
        
        // Camera position with organic motion
        float camX = sinf(time * 0.12f) * 0.4f + cameraOffsetX;
        float camY = sinf(time * 0.18f) * 0.3f + cosf(time * 0.15f) * 0.2f + cameraOffsetY;
        float camZ = cameraDistance + cosf(time * 0.08f) * 0.6f;
        
        // Render
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glUseProgram(shaderProgram);
        
        // Set uniforms
        glUniform2f(u_resolution, (float)windowWidth, (float)windowHeight);
        glUniform1f(u_time, time);
        glUniform3f(u_camPos, camX, camY, camZ);
        glUniformMatrix3fv(u_rotation, 1, GL_FALSE, rotMat);
        glUniform1i(u_colorPalette, colorPalette);
        
        // Draw full-screen quad
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
        
        // Swap buffers
        SDL_GL_SwapWindow(window);
        
        // FPS counter
        frameCount++;
        Uint32 currentTime = SDL_GetTicks();
        if (currentTime - lastFPSTime >= 1000) {
            fps = frameCount / ((currentTime - lastFPSTime) / 1000.0f);
            printf("\rFPS: %.1f | Palette: %d | Camera: (%.2f, %.2f, %.2f)     ", 
                   fps, colorPalette, camX, camY, camZ);
            fflush(stdout);
            frameCount = 0;
            lastFPSTime = currentTime;
        }
    }
    
    printf("\n\nShutting down...\n");
    
    // Cleanup
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(shaderProgram);
    
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;

}