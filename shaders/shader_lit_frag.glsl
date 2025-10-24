#version 410
in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragTexCoord;

layout(std140) uniform Material // Must match the GPUMaterial defined in src/mesh.h
{
    vec3 kd;
    vec3 ks;
    float shininess;
    float transparency;
    float roughness;   
    float metallic;
};

uniform vec3 lightPos, lightColor;
uniform vec3 viewPos;
uniform bool useDiffuse;
uniform sampler2D colorMap;
uniform bool hasTexCoords;
uniform bool useMaterial;
uniform int  shadingMode;// 0=Default, 1=Lambert, 2=Phong, 3=Blinn

uniform samplerCube skybox;


layout(location = 0) out vec4 fragColor;

const float PI = 3.14159265359;

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
    return pow(max(dot(h, n), 0.0), shin);
}

float D_GGX(float NdotH, float r) {
    float a  = r*r;
    float a2 = a*a;
    float NdotH2 = NdotH*NdotH;
    float denom = PI * pow(NdotH2 * (a2 - 1.0) + 1.0, 2.0);
    return a2 / max(denom, 1e-4);
}
float G_SchlickGGX(float NdotX, float r) {
    float k = pow(r + 1.0, 2.0) / 8.0;
    return NdotX / max(NdotX * (1.0 - k) + k, 1e-4);
}
float G_Smith(float NdotV, float NdotL, float r) {
    return G_SchlickGGX(NdotV, r) * G_SchlickGGX(NdotL, r);
}
vec3  F_Schlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

void main() {
    vec3 normal  = normalize(fragNormal);
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
    } else if (shadingMode == 5){ // PBR (GGX)
        float r = clamp(roughness, 0.04, 1.0);
        float m = clamp(metallic, 0.0, 1.0);

        vec3  halfVec = normalize(viewDir + lightDir);
        float NdotL = max(dot(normal, lightDir), 0.0);
        float NdotV = max(dot(normal, viewDir ), 0.0);
        float NdotH = max(dot(normal, halfVec ), 0.0);
        float HdotV = max(dot(halfVec, viewDir), 0.0);

        vec3  F0 = mix(vec3(0.04), albedo, m);
        float D  = D_GGX(NdotH, r);
        float G  = G_Smith(NdotV, NdotL, r);
        vec3  F  = F_Schlick(HdotV, F0);

        vec3 kS = F;
        vec3 kD = (1.0 - kS) * (1.0 - m);

        vec3 spec = (D * G * F) / max(4.0 * NdotL * NdotV, 1e-4);
        vec3 Lo   = (kD * albedo / PI + spec) * NdotL;

        vec3 ambient = albedo * 0.15; 
        color = ambient + Lo;
    } else {
        color = normal;
    }

    color *= lightColor;

    //    vec3 I = normalize(fragPosition - viewPos);
    //    vec3 R = reflect(I, normalize(fragNormal));
    //    color *= vec3(texture(skybox, R).rgb);

    if (hasTexCoords || useMaterial) { fragColor = vec4(clamp(color, 0.0, 1.0), transparency); }
    else { fragColor = vec4(normal, 1); }
}