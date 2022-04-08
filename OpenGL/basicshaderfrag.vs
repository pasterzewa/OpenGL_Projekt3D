#version 330 core
out vec4 FragColor;

in float visibility;

uniform vec3 color;
uniform vec3 skyColor;

void main()
{
    FragColor = vec4(color, 1.0);
    FragColor = mix(vec4(skyColor,1.0), FragColor, visibility);
}