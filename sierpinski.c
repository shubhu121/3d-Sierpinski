#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <SDL2/SDL_opengl.h>

// Embedded vertex shader
const char* vertexShaderSource = 
"#version 330 core\n"
"layout(location = 0) in vec2 aPos;\n"
"void main() {\n"
"    gl_Position = vec4(aPos, 0.0, 1.0);\n"
"}\n";

// Embedded fragment shader
const char* fragmentShaderSource =
"#version 330 core\n"
"out vec4 FragColor;\n"
"uniform vec2 u_resolution;\n"
"uniform float u_time;\n"
"\n"
"// Rotation matrix around Y-axis\n"
"mat3 rotateY(float angle) {\n"
"    float c = cos(angle);\n"
"    float s = sin(angle);\n"
"    return mat3(c, 0.0, s, 0.0, 1.0, 0.0, -s, 0.0, c);\n"
"}\n"
"\n"
"// Sierpinski tetrahedron SDF using iterative folding\n"
"float sierpinskiSDF(vec3 p) {\n"
"    const int iterations = 12;\n"
"    const float scale = 2.0;\n"
"    vec3 a1 = vec3(1.0, 1.0, 1.0);\n"
"    vec3 a2 = vec3(-1.0, -1.0, 1.0);\n"
"    vec3 a3 = vec3(1.0, -1.0, -1.0);\n"
"    vec3 a4 = vec3(-1.0, 1.0, -1.0);\n"
"    vec3 c;\n"
"    float dist, d;\n"
"    int n = 0;\n"
"    \n"
"    // Iterative folding to create Sierpinski structure\n"
"    for (n = 0; n < iterations; n++) {\n"
"        c = a1; dist = length(p - a1);\n"
"        d = length(p - a2); if (d < dist) { c = a2; dist = d; }\n"
"        d = length(p - a3); if (d < dist) { c = a3; dist = d; }\n"
"        d = length(p - a4); if (d < dist) { c = a4; dist = d; }\n"
"        p = scale * p - c * (scale - 1.0);\n"
"    }\n"
"    \n"
"    // Return scaled distance estimate\n"
"    return length(p) * pow(scale, float(-n));\n"
"}\n"
"\n"
"// Ray marching function\n"
"float rayMarch(vec3 ro, vec3 rd, out int steps, out float totalDist) {\n"
"    const int maxSteps = 256;\n"
"    const float maxDist = 20.0;\n"
"    const float epsilon = 0.001;\n"
"    \n"
"    float t = 0.0;\n"
"    steps = 0;\n"
"    \n"
"    for (int i = 0; i < maxSteps; i++) {\n"
"        vec3 pos = ro + rd * t;\n"
"        float dist = sierpinskiSDF(pos);\n"
"        \n"
"        if (dist < epsilon) {\n"
"            totalDist = t;\n"
"            return dist;\n"
"        }\n"
"        \n"
"        t += dist * 0.5; // Relaxation factor for safety\n"
"        steps++;\n"
"        \n"
"        if (t > maxDist) break;\n"
"    }\n"
"    \n"
"    totalDist = t;\n"
"    return -1.0;\n"
"}\n"
"\n"
"// Calculate surface normal using finite differences\n"
"vec3 calcNormal(vec3 p) {\n"
"    const float h = 0.0001;\n"
"    const vec2 k = vec2(1.0, -1.0);\n"
"    return normalize(\n"
"        k.xyy * sierpinskiSDF(p + k.xyy * h) +\n"
"        k.yyx * sierpinskiSDF(p + k.yyx * h) +\n"
"        k.yxy * sierpinskiSDF(p + k.yxy * h) +\n"
"        k.xxx * sierpinskiSDF(p + k.xxx * h)\n"
"    );\n"
"}\n"
"\n"
"// Ambient occlusion approximation\n"
"float calcAO(vec3 p, vec3 n) {\n"
"    float occ = 0.0;\n"
"    float sca = 1.0;\n"
"    for (int i = 0; i < 5; i++) {\n"
"        float h = 0.01 + 0.12 * float(i) / 4.0;\n"
"        float d = sierpinskiSDF(p + h * n);\n"
"        occ += (h - d) * sca;\n"
"        sca *= 0.95;\n"
"    }\n"
"    return clamp(1.0 - 3.0 * occ, 0.0, 1.0);\n"
"}\n"
"\n"
"void main() {\n"
"    // Normalized pixel coordinates\n"
"    vec2 uv = (gl_FragCoord.xy - 0.5 * u_resolution) / u_resolution.y;\n"
"    \n"
"    // Camera setup - orbit around fractal\n"
"    vec3 ro = vec3(0.0, 0.0, 4.5); // Camera position\n"
"    vec3 lookAt = vec3(0.0, 0.0, 0.0);\n"
"    \n"
"    // Camera basis vectors\n"
"    vec3 forward = normalize(lookAt - ro);\n"
"    vec3 right = normalize(cross(vec3(0.0, 1.0, 0.0), forward));\n"
"    vec3 up = cross(forward, right);\n"
"    \n"
"    // Ray direction\n"
"    vec3 rd = normalize(uv.x * right + uv.y * up + 1.5 * forward);\n"
"    \n"
"    // Rotate the ray to rotate the fractal\n"
"    mat3 rot = rotateY(u_time * 0.3);\n"
"    rd = rot * rd;\n"
"    ro = rot * ro;\n"
"    \n"
"    // Ray march\n"
"    int steps;\n"
"    float totalDist;\n"
"    float hit = rayMarch(ro, rd, steps, totalDist);\n"
"    \n"
"    vec3 color = vec3(0.0);\n"
"    \n"
"    if (hit > -0.5) {\n"
"        // Hit surface\n"
"        vec3 pos = ro + rd * totalDist;\n"
"        vec3 normal = calcNormal(pos);\n"
"        \n"
"        // Lighting\n"
"        vec3 lightDir = normalize(vec3(0.5, 0.8, 0.3));\n"
"        float diff = max(dot(normal, lightDir), 0.0);\n"
"        \n"
"        // Specular\n"
"        vec3 viewDir = normalize(-rd);\n"
"        vec3 halfDir = normalize(lightDir + viewDir);\n"
"        float spec = pow(max(dot(normal, halfDir), 0.0), 32.0);\n"
"        \n"
"        // Ambient occlusion\n"
"        float ao = calcAO(pos, normal);\n"
"        \n"
"        // Dynamic color based on iteration depth and time\n"
"        float stepRatio = float(steps) / 256.0;\n"
"        vec3 baseColor = vec3(\n"
"            0.5 + 0.5 * sin(stepRatio * 6.28 + u_time),\n"
"            0.5 + 0.5 * sin(stepRatio * 6.28 + u_time + 2.09),\n"
"            0.5 + 0.5 * sin(stepRatio * 6.28 + u_time + 4.18)\n"
"        );\n"
"        \n"
"        // Combine lighting\n"
"        vec3 ambient = vec3(0.1) * ao;\n"
"        vec3 diffuse = baseColor * diff;\n"
"        vec3 specular = vec3(1.0) * spec * 0.3;\n"
"        \n"
"        color = ambient + (diffuse + specular) * ao;\n"
"        \n"
"        // Fog/depth attenuation\n"
"        float fog = exp(-totalDist * 0.12);\n"
"        color = mix(vec3(0.0), color, fog);\n"
"    }\n"
"    \n"
"    // Gamma correction\n"
"    color = pow(color, vec3(0.4545));\n"
"    \n"
"    FragColor = vec4(color, 1.0);\n"
"}\n";

// Shader compilation helper
GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        fprintf(stderr, "Shader compilation failed:\n%s\n", infoLog);
    }
    
    return shader;
}

// Program linking helper
GLuint createShaderProgram(const char* vertSrc, const char* fragSrc) {
    GLuint vertShader = compileShader(GL_VERTEX_SHADER, vertSrc);
    GLuint fragShader = compileShader(GL_FRAGMENT_SHADER, fragSrc);
    
    GLuint program = glCreateProgram();
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);
    
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        fprintf(stderr, "Program linking failed:\n%s\n", infoLog);
    }
    
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);
    
    return program;
}

int main(int argc, char* argv[]) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
        return 1;
    }
    
    // OpenGL attributes
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    
    // Create window
    SDL_Window* window = SDL_CreateWindow(
        "Sierpinski Tetrahedron - Ray Marching",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1280, 720,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN
    );
    
    if (!window) {
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    
    // Create OpenGL context
    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (!context) {
        fprintf(stderr, "OpenGL context creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    // Initialize GLEW
    glewExperimental = GL_TRUE;
    GLenum glewStatus = glewInit();
    if (glewStatus != GLEW_OK) {
        fprintf(stderr, "GLEW initialization failed: %s\n", glewGetErrorString(glewStatus));
        SDL_GL_DeleteContext(context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    // Create shader program
    GLuint shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);
    
    // Full-screen quad vertices (two triangles)
    float quadVertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
        -1.0f,  1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f
    };
    
    // Create VAO and VBO
    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    // Get uniform locations
    GLint timeLocation = glGetUniformLocation(shaderProgram, "u_time");
    GLint resolutionLocation = glGetUniformLocation(shaderProgram, "u_resolution");
    
    // Timing setup
    Uint64 startTime = SDL_GetPerformanceCounter();
    Uint64 frequency = SDL_GetPerformanceFrequency();
    
    // Main loop
    bool running = true;
    SDL_Event event;
    
    while (running) {
        // Handle events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                running = false;
            }
        }
        
        // Calculate time
        Uint64 currentTime = SDL_GetPerformanceCounter();
        float time = (float)(currentTime - startTime) / (float)frequency;
        
        // Get window size
        int width, height;
        SDL_GetWindowSize(window, &width, &height);
        
        // Clear and render
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glUseProgram(shaderProgram);
        glUniform1f(timeLocation, time);
        glUniform2f(resolutionLocation, (float)width, (float)height);
        
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
        
        SDL_GL_SwapWindow(window);
    }
    
    // Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);
    
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}
