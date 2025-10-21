#version 410
in vec3 fragPosition;
in vec3 fragNormal;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 kd;

out vec4 outColor;

void main(){
    vec3 N = normalize(fragNormal);
    vec3 L = normalize(lightPos - fragPosition);
    float NdotL = max(dot(N,L), 0.0);
    vec3 diff = kd * NdotL;
    vec3 color = lightColor * diff;
    outColor = vec4(color, 1.0);
}
