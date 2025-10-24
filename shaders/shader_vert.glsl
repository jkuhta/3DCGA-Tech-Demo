#version 410

uniform mat4 mvpMatrix;
uniform mat4 modelMatrix;
// Normals should be transformed differently than positions:
// https://paroj.github.io/gltut/Illumination/Tut09%20Normal%20Transformation.html
uniform mat3 normalModelMatrix;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;
layout(location=3) in vec3 tangent;    
layout(location=4) in vec3 bitangent;  

uniform bool hasTangents; 

out vec3 fragPosition;
out vec3 fragNormal;
out vec2 fragTexCoord;
out mat3 fragTBN;

void main()
{
    gl_Position = mvpMatrix * vec4(position, 1);

    fragPosition    = (modelMatrix * vec4(position, 1)).xyz;
    vec3 N = normalize(normalModelMatrix * normal);
    vec3 T = normalize(normalModelMatrix * tangent);
    vec3 B = normalize(normalModelMatrix * bitangent);

    if (!hasTangents) {
        vec3 up = abs(N.y) < 0.999 ? vec3(0,1,0) : vec3(1,0,0);
        T = normalize(cross(up, N));
        B = normalize(cross(N, T));
    }

    T = normalize(T - N * dot(N, T));
    B = normalize(cross(N, T));

    fragNormal   = N;
    fragTBN      = mat3(T, B, N);
    fragTexCoord = texCoord;
}
