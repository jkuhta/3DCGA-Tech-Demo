#version 410
in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragTexCoord;
in mat3 fragTBN;

layout(std140) uniform Material// Must match the GPUMaterial defined in src/mesh.h
{
    vec3 kd;
    vec3 ks;
    float shininess;
    float transparency;
};

uniform vec3 lightPos, lightColor;
uniform vec3 viewPos;
uniform bool useDiffuse;
uniform sampler2D colorMap;
uniform bool hasTexCoords;
uniform bool useMaterial;
uniform int  shadingMode;// 0=Default, 1=Lambert, 2=Phong, 3=Blinn

uniform bool useNormalMap;
uniform sampler2D normalMap;
uniform float normalStrength; 
uniform bool normalFlipY; 

layout(location = 0) out vec4 fragColor;

vec3 computeAlbedo() {
    return (hasTexCoords && !useMaterial) ? texture(colorMap, fragTexCoord).rgb : kd;
}

float lambertTerm(vec3 n, vec3 l) {
    return max(dot(n, l), 0.0);
}

vec3 mapNormal() {
    vec3 n = texture(normalMap, fragTexCoord).rgb * 2.0 - 1.0;
    if (normalFlipY) n.g = -n.g;
    n = normalize(mix(vec3(0,0,1), n, normalStrength));
    return normalize(fragTBN * n);
}

float phongSpecular(vec3 n, vec3 l, vec3 v, float shin) {
    vec3 r = reflect(-l, n);
    return pow(max(dot(r, v), 0.0), shin);
}

float blinnSpecular(vec3 n, vec3 l, vec3 v, float shin) {
    vec3 h = normalize(l + v);
    return pow(max(dot(h, n), 0.0), shin);
}

void main() {
    vec3 normal  = normalize(fragNormal);

    if (useNormalMap && hasTexCoords)
        normal = mapNormal();
    vec3 lightDir= normalize(lightPos - fragPosition);
    vec3 viewDir = normalize(viewPos  - fragPosition);

    vec3  albedo = computeAlbedo();
    float diff = lambertTerm(normal, lightDir);
    float blinnSpec = blinnSpecular(normal, lightDir, viewDir, shininess);

    vec3 color = vec3(0.0);

    if (shadingMode == 0) { // Default (Lambert + BlinnPhong)
        color = albedo * diff + ks * blinnSpec;
    } else if (shadingMode == 1) { // Albedo
        color = albedo;
    } else if (shadingMode == 2) { // Lambert
        color = albedo * diff;
    } else if (shadingMode == 3) { // Phong
        if (useDiffuse) color += albedo * diff;
        color += ks * phongSpecular(normal, lightDir, viewDir, shininess);
    } else if (shadingMode == 4){ // Blinn-Phong
        if (useDiffuse) color += albedo * diff;
        color += ks * blinnSpec;
    } else {
        color = normal;
    }

    color *= lightColor;

    if (hasTexCoords || useMaterial) { fragColor = vec4(clamp(color, 0.0, 1.0), transparency); }
    else { fragColor = vec4(normal, 1); }// Output color value, change from (1, 0, 0) to something else
}
