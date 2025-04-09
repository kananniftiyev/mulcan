#version 450

// Input attributes (matching your vertex input bindings)
layout(location = 0) in vec3 inPosition; // Vertex position
layout(location = 1) in vec3 inColor;    // Vertex color

// Output to the fragment shader
layout(location = 0) out vec3 fragColor;

layout( push_constant ) uniform constants
{
	vec4 data;
	mat4 render_matrix;
} PushConstants;

void main() {
    gl_Position = PushConstants.render_matrix * vec4(inPosition, 1.0f);
    fragColor = inColor;                // Pass color to fragment shader
}
