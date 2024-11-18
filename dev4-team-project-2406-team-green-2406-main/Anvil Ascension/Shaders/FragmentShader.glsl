#version 330 // GLSL 3.30

in vec3 fragPos;
in vec3 worldNorm;
in vec2 TexCoords;

out vec4 FragColor;

uniform sampler2D texture_diffuse;
uniform vec3 ambientColor;
uniform vec3 lightDir;    // Light direction
uniform vec3 lightColor;  // Light color
uniform vec3 mapCenter;

void main()
{
    // Normalize the normal vector
    vec3 norm = normalize(worldNorm);
    
    // Calculate the diffuse lighting
    float diff = max(dot(norm, lightDir), 0.0);
    
    // Sample color from the texture
    vec3 textureColor = texture(texture_diffuse, TexCoords).rgb;

    // Calculate the ambient and diffuse components
    vec3 ambient = textureColor * ambientColor;
    vec3 diffuse = diff * textureColor * lightColor;

    // Combine ambient and diffuse components
    vec3 color = ambient + diffuse;

    // Output the final color
    FragColor = vec4(color, 1.0);
}