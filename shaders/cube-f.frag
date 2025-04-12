#version 450

// Input from the vertex shader
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTex;
layout(location = 3) in vec3 fragNormals;

// Output to the framebuffer
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0); 
}
