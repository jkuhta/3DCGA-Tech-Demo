#version 410
in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragTexCoord;

uniform vec3 lightPos, lightColor;
uniform vec3 kd, ks;
uniform float shininess;
uniform vec3 viewPos;
uniform bool useDiffuse;
uniform sampler2D colorMap;
uniform bool hasTexCoords;
uniform bool useMaterial;
uniform int  shadingMode;   // 0=Default, 1=Lambert, 2=Phong, 3=Blinn

out vec4 outColor;

vec3 computeAlbedo() {
    return (hasTexCoords && !useMaterial) ? texture(colorMap, fragTexCoord).rgb : kd;
}

float lambertTerm(vec3 n, vec3 l) {
    return max(dot(n, l), 0.0);
}

float phongSpecular(vec3 n, vec3 l, vec3 v, float shin) {
    vec3 r = reflect(-l, n);
    return pow(max(dot(r, v), 0.0), shin);
}

float blinnSpecular(vec3 n, vec3 l, vec3 v, float shin) {
    vec3 h = normalize(l + v);
    return pow(max(dot(n, h), 0.0), shin);
}

void main() {
    vec3 normal  = normalize(fragNormal);
    vec3 lightDir= normalize(lightPos - fragPosition);
    vec3 viewDir = normalize(viewPos  - fragPosition);

    float NdL = lambertTerm(normal, lightDir);
    vec3  albedo = computeAlbedo();

    vec3 color = vec3(0.0);

    if (shadingMode == 0) {                 // Default (unlit)
        color = albedo;
    } else if (shadingMode == 1) {          // Lambert
        color = albedo * NdL;
    } else if (shadingMode == 2) {          // Phong
        if (useDiffuse) color += albedo * NdL;
        if (NdL > 0.0)  color += ks * phongSpecular(normal, lightDir, viewDir, shininess);
    } else {                                // Blinn-Phong
        if (useDiffuse) color += albedo * NdL;
        if (NdL > 0.0)  color += ks * blinnSpecular(normal, lightDir, viewDir, shininess);
    }

    color *= lightColor;
    outColor = vec4(color, 1.0);
}
