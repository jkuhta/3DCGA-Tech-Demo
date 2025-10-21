#version 410
in vec3 fragPosition;
in vec3 fragNormal;

uniform vec3 lightPos, lightColor;
uniform vec3 ks;
uniform float shininess;
uniform vec3 viewPos;

out vec4 outColor;

void main(){
    vec3 N = normalize(fragNormal);
    vec3 L = normalize(lightPos - fragPosition);
    vec3 V = normalize(viewPos  - fragPosition);
    vec3 H = normalize(L + V);

    float NdotL = max(dot(N,L), 0.0);
    float spec  = (NdotL > 0.0) ? pow(max(dot(N,H),0.0), shininess) : 0.0;

    vec3 color = lightColor * (ks * spec);
    outColor = vec4(color, 1.0);
}
