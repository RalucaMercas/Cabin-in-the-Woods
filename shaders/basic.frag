#version 410 core
#define NUM_POINT_LIGHTS 10

in vec3 fPosition;
in vec3 fNormal;
in vec2 fTexCoords;
in vec4 fragPosLightSpace;

out vec4 fColor;

uniform mat4 model;
uniform mat4 view;
uniform mat3 normalMatrix;

uniform vec3 lightDir;
uniform vec3 lightColor;

uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
uniform sampler2D shadowMap;

uniform vec3 pointLightPositions[NUM_POINT_LIGHTS];
uniform vec3 pointLightColor;
uniform float constantAttenuation;
uniform float linearAttenuation;
uniform float quadraticAttenuation;

vec3 ambient;
float ambientStrength = 0.2f;
vec3 diffuse;
vec3 specular;
float specularStrength = 0.5f;

void computeDirLight() {
    vec4 fPosEye = view * model * vec4(fPosition, 1.0f);
    vec3 normalEye = normalize(normalMatrix * fNormal);
    vec3 lightDirN = vec3(normalize(vec4(lightDir, 0.0f)));
    vec3 viewDir = normalize(- fPosEye.xyz);
    ambient = ambientStrength * lightColor;
    diffuse = max(dot(normalEye, lightDirN), 0.0f) * lightColor;
    vec3 reflectDir = reflect(-lightDirN, normalEye);
    float specCoeff = pow(max(dot(viewDir, reflectDir), 0.0f), 32);
    specular = specularStrength * specCoeff * lightColor;
}

float computeShadow() {
    vec3 normalizedCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    normalizedCoords = normalizedCoords * 0.5 + 0.5;
    if(normalizedCoords.z > 1.0f) {
        return 0.0f;
    }
    float closestDepth = texture(shadowMap, normalizedCoords.xy).r;
    float currentDepth = normalizedCoords.z;
    float bias = 0.005f;
    float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
    return shadow;
}

uniform vec3 fogColor; 
uniform float fogDensity; 

vec4 applyFog(vec4 fragmentColor, vec3 fragmentPosEyeSpace) {
    float fragmentDistance = length(fragmentPosEyeSpace);
    float fogFactor = exp(-pow(fragmentDistance * fogDensity, 2.0));
    fogFactor = clamp(fogFactor, 0.0, 1.0);

    return mix(vec4(fogColor, 1.0), fragmentColor, fogFactor);
}

vec3 computePointLight(int index, vec3 fragPos, vec3 normal, vec3 viewDir) {
    vec3 lightDir = normalize(pointLightPositions[index] - fragPos);
    float distance = length(pointLightPositions[index] - fragPos);
    float attenuation = 1.0 / (constantAttenuation + linearAttenuation * distance + quadraticAttenuation * (distance * distance));
    
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * pointLightColor; 
    
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = spec * pointLightColor; 
    
    return attenuation * (diffuse + specular);
}


void main() 
{
    computeDirLight();
    
    float shadow = computeShadow();
    
    vec3 dirLightColor = min(
        (ambient + (1.0 - shadow) * diffuse) * texture(diffuseTexture, fTexCoords).rgb + 
        (1.0 - shadow) * specular * texture(specularTexture, fTexCoords).rgb, 
        1.0f
    );

    vec3 fragPosEyeSpace = (view * vec4(fPosition, 1.0)).xyz;

    vec3 viewDir = normalize(-fragPosEyeSpace);
    
    vec3 pointLightColor = vec3(0.0);
    for (int i = 0; i < NUM_POINT_LIGHTS; i++) {
        pointLightColor += computePointLight(i, fPosition, fNormal, viewDir);
    }
    
    vec3 finalColorComponents = dirLightColor + pointLightColor;

    vec4 finalColor = applyFog(vec4(finalColorComponents, 1.0f), fragPosEyeSpace);
    
    fColor = finalColor;
}
