#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(std140, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 light;
	vec4 Ka;
	vec4 Kd;
	vec4 Ks;
	vec4 Ke;
    float Ns;
	float model_stage_on;
	float texture_stage_on;
	float lighting_stage_on;
    float specular;
	float diffuse;
	float ambient;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out float texture_on;
layout(location = 4) out float specularity;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);

    outNormal = inNormal;

    specularity = 0.0;

    if(ubo.model_stage_on > 0)
    {
        if(ubo.lighting_stage_on > 0)
        {
            vec3 vert_pos = (ubo.model * vec4(inPosition, 1.0)).xyz;
            vec3 normal_dir = normalize((mat3(ubo.model) * inNormal).xyz);
            vec3 light_pos = (ubo.light * vec4(-5.0, 0.0, 0.0, 1.0)).xyz;
            vec3 light_dir = normalize(vert_pos - light_pos);

            float ambient = ubo.ambient;
            float diffuse = ubo.diffuse * max(0.0, dot(normal_dir, -light_dir));
            
            float specular = 0.0;
            if(diffuse != 0.0)
            {
                vec3 camera_dir = normalize(vert_pos - vec3(-2.0, 0.0, 0.0));
                vec3 reflection_dir = normalize(reflect(light_dir, normal_dir));

                float spec_val = pow(max(dot(reflection_dir, -camera_dir), 0.0), ubo.Ns);
                specular = clamp(ubo.specular * spec_val, 0.0, 1.0);
            }

            specularity = specular;

            fragColor = clamp(ubo.Ke.xyz + inColor * (ambient * ubo.Ka.xyz + diffuse * ubo.Kd.xyz + specular * ubo.Ks.xyz), vec3(0.0), vec3(1.0));
         }
         else
         {
            fragColor = vec3(0.7, 0.7, 0.7);
         }
    }
    else
    {
        fragColor = vec3(0.0,0.0,0.0);
    }

    fragTexCoord = inTexCoord;
    texture_on = int(ubo.texture_stage_on);
}