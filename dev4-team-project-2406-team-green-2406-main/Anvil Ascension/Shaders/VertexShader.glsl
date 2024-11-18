#version 330 core


layout (std140) uniform UboData {
    vec4 sunDirection;
    vec4 sunColor;
    mat4 viewMatrix;
    mat4 projectionMatrix;
    mat4 worldMatrix;
};

// Input from VBO
layout(location = 0) in vec3 localPos;
layout(location = 1) in vec2 localTexCoords; // Changed to vec2 for texture coordinates
layout(location = 2) in vec3 localNorm;

// Output to Fragment shader
out vec3 worldNorm;
out vec3 fragPos;
out vec2 TexCoords;

void main()
{
    // Transform position to world space
    vec4 worldPosition = worldMatrix * vec4(localPos, 1.0);

    // Transform normal to world space using the correct method
    worldNorm = mat3(transpose(inverse(worldMatrix))) * localNorm;

    fragPos = worldPosition.xyz;

    // Pass through the texture coordinates
    TexCoords = localTexCoords;

    // Transform position to view space
    vec4 viewPosition = viewMatrix * worldPosition;

    // Transform position to clip space
    gl_Position = projectionMatrix * viewPosition;
}