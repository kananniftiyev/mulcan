#version 450

// Input attributes (matching your vertex input bindings)
layout(location = 0) in vec3 inPosition; // Vertex position
layout(location = 1) in vec3 inColor;    // Vertex color

// Output to the fragment shader
layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = vec4(inPosition, 1.0); // Transform position to clip space
    fragColor = inColor;                // Pass color to fragment shader
}
