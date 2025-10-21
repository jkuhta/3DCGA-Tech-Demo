#version 410
in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragTexCoord;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 kd;
uniform sampler2D colorMap;          
uniform bool hasTexCoords;           
uniform bool useMaterial; 

out vec4 outColor;

void main(){
    vec3 N = normalize(fragNormal);
    vec3 L = normalize(lightPos - fragPosition);
    float NdotL = max(dot(N,L), 0.0);

    vec3 albedo = (hasTexCoords && !useMaterial)
                  ? texture(colorMap, fragTexCoord).rgb
                  : kd;                                

    vec3 color = lightColor * (albedo * NdotL);
    outColor = vec4(color, 1.0);
}
