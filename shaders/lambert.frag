#version 330 core
in vec3 vFragPosition;
in vec2 vTexCoords;
in vec3 vNormal;
in vec3 skyBoxTex;
in mat3 TBN;


struct Material {
    sampler2D diffuse;
    sampler2D specular;
    vec3 color;
    float shininess;
};

struct DirectLight {
    vec3 direction;
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;  
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
	
    float constant;
    float linear;
    float quadratic;
}; 

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
  
    float constant;
    float linear;
    float quadratic;
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;       
};

out vec4 outColor;

uniform vec3 camPos;
uniform sampler2D ourTexture;
uniform int type;
uniform samplerCube skybox;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform Material material;
uniform sampler2D normalMap;
uniform bool useNormalMap;

uniform DirectLight directLight;

#define NR_POINT_LIGHTS 2  
uniform PointLight pointLights[NR_POINT_LIGHTS];

uniform SpotLight spotLight;


vec3 CalcDirLight(DirectLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    // combine results
    vec3 ambient  = light.ambient  * vec3(texture(material.diffuse, vTexCoords));
    vec3 diffuse  = light.diffuse  * diff * vec3(texture(material.diffuse, vTexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.specular, vTexCoords));
    return (ambient + diffuse + specular);
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    // attenuation
    float distance    = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    
    // combine results
    vec3 ambient  = light.ambient  * vec3(texture(material.diffuse, vTexCoords));
    vec3 diffuse  = light.diffuse  * diff * vec3(texture(material.diffuse, vTexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.specular, vTexCoords));
    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
}

vec3 CalcPointLightTBN(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 tangentFragPos)
{
    vec3 lightDir;
    if (useNormalMap){
        vec3 tangentLightPos = TBN * light.position;

        vec3 lightDir = normalize(tangentLightPos - tangentFragPos);
    }
    else {
        lightDir = normalize(light.position - fragPos);
    }
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    //vec3 reflectDir = reflect(-lightDir, normal);

    vec3 halfwayDir = normalize(lightDir + viewDir); 
    float spec = pow(max(dot(viewDir, halfwayDir), 0.0), material.shininess);
    // attenuation
    float distance1 = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance1 + light.quadratic * (distance1 * distance1));    
    // combine results
    vec3 ambient = light.ambient * vec3(texture(material.diffuse, vTexCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, vTexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.specular, vTexCoords));

    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);

}

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    //vec3 reflectDir = reflect(-lightDir, normal);
    vec3 halfwayDir = normalize(lightDir + viewDir); 
    float spec = pow(max(dot(viewDir, halfwayDir), 0.0), material.shininess);
    // attenuation
    float distance1 = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance1 + light.quadratic * (distance1 * distance1));    
    // spotlight intensity
    float theta = dot(lightDir, normalize(-light.direction)); 
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    // combine results
    vec3 ambient = light.ambient * vec3(texture(material.diffuse, vTexCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, vTexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.specular, vTexCoords));
    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
    return (ambient + diffuse + specular);
}

void main()
{
    if (type == 0)
    {
        vec3 lightDir1 = vec3(1.0f, 1.0f, 1.0f);
        vec3 lightDir2 = vec3(-1.0f, 1.0f, 1.0f);
        vec3 lightDir3 = vec3(0.0f, 1.0f, 1.0f);

        vec3 lightColor1 = vec3(0.64f, 1.0f, 1.0f);
        vec3 lightColor2 = vec3(1.0f, 0.56f, 0.001f);
        vec3 lightColor3 = vec3(1.0f, 1.0f, 1.0f);

        vec3 base_color = vec3(0.05f, 0.05f, 0.15f);
        //vec3 base_color = vec3(1.0f, 1.0f, 1.0f);


        vec4 color1 = vec4(abs(dot(vNormal, lightDir1)) * lightColor1, 1.0f);
        vec4 color2 = vec4(abs(dot(vNormal, lightDir2)) * lightColor2, 1.0f);
        vec4 color3 = vec4(abs(dot(vNormal, lightDir3)) * lightColor3, 1.0f);
        vec4 color_l = 0.45f * color1 + 0.45f * color2 + 0.1f * color3;

        outColor = vec4(color_l.xyz * base_color, 1.0f);
    }
    else if (type == 1)
    {
        vec3 color = texture(skybox, skyBoxTex).rgb;
        outColor = vec4(color, 1.0f)/(1+vec4(color, 1.0f));
    }
    else if (type == 2)
    {
        outColor = texture(ourTexture, vTexCoords);
    }
    else if (type == 3)
    {
        vec3 base_color = vec3(1.0f, 1.0f, 1.0f);
        outColor = vec4(base_color * lightColor, 1.0f);
    }
    else if (type == 4)
    {
        vec3 norm = normalize(vNormal);
        float specularStrength = 0.4;
        vec3 lightDir = normalize(pointLights[0].position - vFragPosition);
        float diff = max(dot(norm, lightDir), 0.0);

        float distance    = length(pointLights[0].position - vFragPosition);
        float attenuation = 1.0 / (pointLights[0].constant + pointLights[0].linear * distance + pointLights[0].quadratic * (distance * distance));    

        vec3 viewDir = normalize(camPos - vFragPosition);
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
        vec3 specular = specularStrength * spec * lightColor;
        
        vec3 directColor = vec3(1.0f, 1.0f, 1.0f);
        vec3 directDir = normalize(-directLight.direction);
        float diffDir = max(dot(norm, directDir), 0.0);

        vec3 diffuse = diff * diffDir * lightColor;
        diffuse  *= attenuation;
        specular *= attenuation;
        vec3 result = (specular + diffuse) * material.color;
        outColor = vec4(result, 1.0);
    }
    else if (type == 5)
    {
       vec3 normal = texture(normalMap, vTexCoords).rgb;
       normal = normalize(normal * 2.0 - 1.0);

       vec3 tangentViewPos  = TBN * camPos;
       vec3 tangentFragPos  = TBN * vFragPosition;

       vec3 viewDir = normalize(tangentViewPos - tangentFragPos);

       vec3 color = vec3(0.0f,0.0f,0.0f);
       color += CalcDirLight(directLight, normal, viewDir);

       for (int i = 0; i < NR_POINT_LIGHTS; i++)
       {
            color += CalcPointLightTBN(pointLights[i], vNormal, vFragPosition, viewDir, tangentFragPos);
       }

       vec3 hdrColor = texture(material.diffuse, vTexCoords).rgb;
       vec3 color1 = hdrColor / (hdrColor + vec3(1.0));
       //outColor = vec4(color, 1.0f)/(1+vec4(color, 1.0f));
       outColor = vec4(color, 1.0f);
    }
    else
    {
        vec3 norm = normalize(vNormal);
        vec3 viewDir = normalize(camPos - vFragPosition);

        // phase 1: Directional lighting
        vec3 result = CalcDirLight(directLight, norm, viewDir);
        // phase 2: Point lights
        for(int i = 0; i < NR_POINT_LIGHTS; i++)
            result += CalcPointLight(pointLights[i], norm, vFragPosition, viewDir);    
        // phase 3: Spot light
        //result += CalcSpotLight(spotLight, norm, vFragPosition, viewDir);    
        outColor = vec4(result, 1.0);
    }
}