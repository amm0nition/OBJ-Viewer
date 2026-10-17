#version 460 core
in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

uniform vec3 lightDir;
uniform vec3 viewPos;
uniform vec3 objectColor;
uniform vec3 lightColor;

void main()
{
    vec3 norm = normalize(Normal);
    vec3 light = normalize(lightDir);
    vec3 viewDir = normalize(viewPos - FragPos);

    vec3 ambient = 0.15 * lightColor;

    float NdotL = dot(norm, light);
    float halfLambert = NdotL * 0.5 + 0.5;
    float diffuseFactor = halfLambert * halfLambert;
    vec3 diffuse = diffuseFactor * lightColor;

    vec3 reflectDir = reflect(-light, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = 0.45 * spec * lightColor;

    vec3 result = (ambient + diffuse) * objectColor + specular;
    FragColor = vec4(result, 1.0);
}
