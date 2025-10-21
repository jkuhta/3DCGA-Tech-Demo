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

out vec4 outColor;

void main() {
    vec3 N = normalize(fragNormal);
    vec3 L = normalize(lightPos - fragPosition);
    vec3 V = normalize(viewPos - fragPosition);

    float NdL = max(dot(N, L), 0.0);
    vec3 albedo = (hasTexCoords && !useMaterial)
                  ? texture(colorMap, fragTexCoord).rgb
                  : kd;

    vec3 color = vec3(0.0);
    if (useDiffuse) color += albedo * NdL;

    if (NdL > 0.0) {
        vec3 R = reflect(-L, N);
        float RdV = max(dot(R, V), 0.0);
        color += ks * pow(RdV, shininess);
    }

    color *= lightColor;
    outColor = vec4(color, 1.0);
}

