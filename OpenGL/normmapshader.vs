#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    mat3 TBN;
    vec3 TangentFragPos;
    float visibility;
} vs_out;

uniform mat4 model;
uniform mat4 invmodel;
uniform mat4 view;
uniform mat4 projection;

const float density = 0.02;
const float gradient = 3.5;

void main()
{
	vs_out.FragPos = vec3(model * vec4(aPos, 1.0));
    vs_out.TexCoords = aTexCoords;

    mat3 normMatrix = transpose(inverse(mat3(model)));
    vec3 T = normalize(normMatrix * aTangent);
    vec3 N = normalize(normMatrix * aNormal);
    T = normalize(T - dot(T,N) * N);
    vec3 B = cross(N,T);

    mat3 TBN = transpose(mat3(T, B, N));  
    vs_out.TBN = TBN;
    vs_out.TangentFragPos  = TBN * vs_out.FragPos;

    vs_out.Normal = mat3(transpose(invmodel)) * aNormal;
	gl_Position = projection * view * model * vec4(aPos, 1.0);

    vec4 posRelative = view * model * vec4(aPos, 1.0);
    float distance = length(posRelative.xyz); 
	vs_out.visibility = exp(-pow((distance*density),gradient)); 
	vs_out.visibility = clamp(vs_out.visibility, 0.0, 1.0); 
}