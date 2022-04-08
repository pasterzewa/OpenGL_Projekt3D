#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;
out float visibility;

uniform mat4 model;
uniform mat4 invmodel;
uniform mat4 view;
uniform mat4 projection;

const float density = 0.02;
const float gradient = 3.5;

void main()
{
	FragPos = vec3(model * vec4(aPos, 1.0));
	vec4 posRelative = view * model * vec4(aPos, 1.0);
    Normal = mat3(transpose(invmodel)) * aNormal; 
	gl_Position = projection * view * model * vec4(aPos, 1.0);
	float distance = length(posRelative.xyz); 
	visibility = exp(-pow((distance*density),gradient)); 
	visibility = clamp(visibility, 0.0, 1.0); 
}