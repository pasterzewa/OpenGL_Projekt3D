#version 330 core
layout (location = 0) in vec3 aPos;

out float visibility;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

const float density = 0.02;
const float gradient = 3.5;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    vec4 posRelative = view * model * vec4(aPos, 1.0);
    float distance = length(posRelative.xyz);
    visibility = exp(-pow((distance*density),gradient));
	visibility = clamp(visibility, 0.0, 1.0);
}