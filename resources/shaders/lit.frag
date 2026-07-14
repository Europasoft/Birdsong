#version 450
// inputs from vertex shader
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPositionWS;
layout(location = 2) in vec3 fragNormalWS;
layout(location = 3) in vec2 fragUV;
layout (location = 0) out vec4 outColor; // pixel color output

// descriptor set 0 inputs
layout(std140, set = 0, binding = 0) uniform UBO1 
{
	mat4 projectionViewMatrix;
	vec3 cameraPosition;
} ubo1;
layout(set = 0, binding = 1) uniform texture2D textures[2];
layout(set = 0, binding = 2) uniform sampler _sampler;

layout(push_constant) uniform Push
{
	mat4 transform;
	mat4 normalMatrix;
} push;

void main()
{
	//vec3 dirToLight = globalFrameData.lightPosition - fragPositionWS;
	//float attenuation = 1.0 / dot(dirToLight, dirToLight);

	//vec3 lightColor = globalFrameData.lightColor.xyz * globalFrameData.lightColor.w * attenuation;
	//vec3 lightAmbient = globalFrameData.ambientLightColor.xyz * globalFrameData.ambientLightColor.w;
	//vec3 lightDiffuse = lightColor * max(dot(normalize(fragNormalWS), normalize(dirToLight)), 0);

	//outColor = vec4((lightDiffuse + lightAmbient) * fragColor, 1.0);
	//outColor = vec4(fragUV.x * ubo2.hue.x, fragUV.y * ubo2.hue.y, 0.5 * ubo2.hue.z, 1.0); // test uv coords
	//outColor = texture(texSampler, fragUV);
	outColor = texture(sampler2D(textures[0], _sampler), fragUV);

	// PBR ---------------------------------------------------------------------------------
	vec3 N = normalize(fragNormalWS);
    vec3 V = normalize(ubo1.cameraPosition - fragPositionWS);

    vec3 F0 = vec3(0.04); // base reflectance
    F0 = mix(F0, albedo, metallic);
	           
    // reflectance equation
    vec3 Lo = vec3(0.0);
    for(int i = 0; i < 4; ++i) 
    {
        // calculate per-light radiance
        vec3 L = normalize(lightPositions[i] - WorldPos);
        vec3 H = normalize(V + L);
        float distance    = length(lightPositions[i] - WorldPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance     = lightColors[i] * attenuation;        
        
        // cook-torrance brdf
        float NDF = DistributionGGX(N, H, roughness);        
        float G   = GeometrySmith(N, V, L, roughness);      
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);       
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;	  
        
        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular     = numerator / denominator;  
            
        // add to outgoing radiance Lo
        float NdotL = max(dot(N, L), 0.0);                
        Lo += (kD * albedo / PI + specular) * radiance * NdotL; 
    }   
  
    vec3 ambient = vec3(0.03) * albedo;
    vec3 color = ambient + Lo;
	
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));  
   
    FragColor = vec4(color, 1.0);
}