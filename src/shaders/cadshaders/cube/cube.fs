#version 330 core

in vec3 FragPos;
in vec3 Normal;

uniform vec4 color;
uniform vec3 lightPos;
uniform vec3 viewPos;

out vec4 FragColor;

void main() {
    // 环境光
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * color.rgb;
    
    // 漫反射
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * color.rgb;
    
    // 镜面反射
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * vec3(1.0);
    
    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, color.a);
}