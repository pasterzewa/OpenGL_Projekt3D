#version 330 core
out vec4 FragColor;

in vec3 Color;
in float visibility;

uniform vec3 skyColor;

void main()
{       
    FragColor = vec4(Color, 1.0);
    FragColor = mix(vec4(skyColor,1.0), FragColor, visibility);
}