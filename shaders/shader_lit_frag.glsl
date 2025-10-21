#version 410
in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragTexCoord;

uniform vec3 lightPos, lightColor;
uniform vec3 kd, ks;
uniform float shininess;
uniform vec3 viewPos;
uniform bool useDiffuse;          // add diffuse term in specular modes
uniform sampler2D colorMap;
uniform bool hasTexCoords;
uniform bool useMaterial;
uniform int  shadingMode;         // 0=Default(unlit), 1=Lambert, 2=Phong, 3=Blinn

out vec4 outColor;

vec3 albedo()
{
    return (hasTexCoords && !useMaterial) ? texture(colorMap, fragTexCoord).rgb : kd;
}

void main() {
    vec3 N = normalize(fragNormal);
    vec3 L = normalize(lightPos - fragPosition);
    vec3 V = normalize(viewPos - fragPosition);
    vec3 H = normalize(L + V);
    float NdL = max(dot(N, L), 0.0);

    vec3 C = vec3(0.0);
    vec3 A = albedo();

    if (shadingMode == 0) { // Default (unlit textured or kd)
        C = A;
    }
    else if (shadingMode == 1) { // Lambert
        C = A * NdL;
    }
    else if (shadingMode == 2) { // Phong
        if (useDiffuse) C += A * NdL;
        if (NdL > 0.0) {
            vec3 R = reflect(-L, N);
            float RdV = max(dot(R, V), 0.0);
            C += ks * pow(RdV, shininess);
        }
    }
    else { // 3 = Blinn-Phong
        if (useDiffuse) C += A * NdL;
        if (NdL > 0.0) {
            float NdH = max(dot(N, H), 0.0);
            C += ks * pow(NdH, shininess);
        }
    }

    C *= lightColor;
    outColor = vec4(C, 1.0);
}
