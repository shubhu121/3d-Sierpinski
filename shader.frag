#version 330 core
out vec4 FragColor;
uniform vec2 u_resolution;
uniform float u_time;

// Rotation matrix around Y-axis
mat3 rotateY(float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return mat3(c, 0.0, s, 0.0, 1.0, 0.0, -s, 0.0, c);
}

// Sierpinski tetrahedron SDF using iterative folding
float sierpinskiSDF(vec3 p) {
    const int iterations = 12;
    const float scale = 2.0;
    vec3 a1 = vec3(1.0, 1.0, 1.0);
    vec3 a2 = vec3(-1.0, -1.0, 1.0);
    vec3 a3 = vec3(1.0, -1.0, -1.0);
    vec3 a4 = vec3(-1.0, 1.0, -1.0);
    vec3 c;
    float dist, d;
    int n = 0;
    
    // Iterative folding to create Sierpinski structure
    for (n = 0; n < iterations; n++) {
        c = a1; dist = length(p - a1);
        d = length(p - a2); if (d < dist) { c = a2; dist = d; }
        d = length(p - a3); if (d < dist) { c = a3; dist = d; }
        d = length(p - a4); if (d < dist) { c = a4; dist = d; }
        p = scale * p - c * (scale - 1.0);
    }
    
    // Return scaled distance estimate
    return length(p) * pow(scale, float(-n));
}

// Ray marching function
float rayMarch(vec3 ro, vec3 rd, out int steps, out float totalDist) {
    const int maxSteps = 256;
    const float maxDist = 20.0;
    const float epsilon = 0.001;
    
    float t = 0.0;
    steps = 0;
    
    for (int i = 0; i < maxSteps; i++) {
        vec3 pos = ro + rd * t;
        float dist = sierpinskiSDF(pos);
        
        if (dist < epsilon) {
            totalDist = t;
            return dist;
        }
        
        t += dist * 0.5;
        steps++;
        
        if (t > maxDist) break;
    }
    
    totalDist = t;
    return -1.0;
}

// Calculate surface normal using finite differences
vec3 calcNormal(vec3 p) {
    const float h = 0.0001;
    const vec2 k = vec2(1.0, -1.0);
    return normalize(
        k.xyy * sierpinskiSDF(p + k.xyy * h) +
        k.yyx * sierpinskiSDF(p + k.yyx * h) +
        k.yxy * sierpinskiSDF(p + k.yxy * h) +
        k.xxx * sierpinskiSDF(p + k.xxx * h)
    );
}

// Ambient occlusion approximation
float calcAO(vec3 p, vec3 n) {
    float occ = 0.0;
    float sca = 1.0;
    for (int i = 0; i < 5; i++) {
        float h = 0.01 + 0.12 * float(i) / 4.0;
        float d = sierpinskiSDF(p + h * n);
        occ += (h - d) * sca;
        sca *= 0.95;
    }
    return clamp(1.0 - 3.0 * occ, 0.0, 1.0);
}

void main() {
    vec2 uv = (gl_FragCoord.xy - 0.5 * u_resolution) / u_resolution.y;
    
    vec3 ro = vec3(0.0, 0.0, 4.5);
    vec3 lookAt = vec3(0.0, 0.0, 0.0);
    
    vec3 forward = normalize(lookAt - ro);
    vec3 right = normalize(cross(vec3(0.0, 1.0, 0.0), forward));
    vec3 up = cross(forward, right);
    
    vec3 rd = normalize(uv.x * right + uv.y * up + 1.5 * forward);
    
    mat3 rot = rotateY(u_time * 0.3);
    rd = rot * rd;
    ro = rot * ro;
    
    int steps;
    float totalDist;
    float hit = rayMarch(ro, rd, steps, totalDist);
    
    vec3 color = vec3(0.0);
    
    if (hit > -0.5) {
        vec3 pos = ro + rd * totalDist;
        vec3 normal = calcNormal(pos);
        
        vec3 lightDir = normalize(vec3(0.5, 0.8, 0.3));
        float diff = max(dot(normal, lightDir), 0.0);
        
        vec3 viewDir = normalize(-rd);
        vec3 halfDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(normal, halfDir), 0.0), 32.0);
        
        float ao = calcAO(pos, normal);
        
        float stepRatio = float(steps) / 256.0;
        vec3 baseColor = vec3(
            0.5 + 0.5 * sin(stepRatio * 6.28 + u_time),
            0.5 + 0.5 * sin(stepRatio * 6.28 + u_time + 2.09),
            0.5 + 0.5 * sin(stepRatio * 6.28 + u_time + 4.18)
        );
        
        vec3 ambient = vec3(0.1) * ao;
        vec3 diffuse = baseColor * diff;
        vec3 specular = vec3(1.0) * spec * 0.3;
        
        color = ambient + (diffuse + specular) * ao;
        
        float fog = exp(-totalDist * 0.12);
        color = mix(vec3(0.0), color, fog);
    }
    
    color = pow(color, vec3(0.4545));
    
    FragColor = vec4(color, 1.0);
}
