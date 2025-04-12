#version 450

// Input attributes (matching your vertex input bindings)
layout(location = 0) in vec3 inPosition; // Vertex position
layout(location = 1) in vec3 inColor;    // Vertex color
layout(location = 2) in vec2 inTexture; // Vertex texCords
layout(location = 3) in vec3 inNormals; // Vertex normals

// Output to the fragment shader
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTex;
layout(location = 2) out vec3 fragNormals;

layout( push_constant ) uniform constants
{
	vec4 data;
	mat4 render_matrix;
} PushConstants;

layout ( set = 0, binding = 0) uniform CameraBuffer{
    mat4 view;
	mat4 proj;
	mat4 viewproj;
} CameraData;

void main() {
    fragColor = inColor;                
    fragTex = inTexture;
    fragNormals = inNormals;

    mat4 transformMatrix = (cameraData.viewproj * PushConstants.render_matrix);
    gl_Position = transformMatrix * vec4(inPosition, 1.0f);
    
}
