#version 450
#extension GL_KHR_vulkan_glsl : enable

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
	int display_mode;
    vec2 win_dim;
} ubo;

layout (input_attachment_index = 0, set = 0, binding = 1) uniform subpassInput inColor;
layout (input_attachment_index = 0, set = 0, binding = 2) uniform subpassInput inNormal;
layout (input_attachment_index = 0, set = 0, binding = 4) uniform subpassInput inDepth;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragcolor;

const vec4 lights[2] = { vec4(-5.0, 0.0, 0.0, 1.0), vec4(5.0, 0.0, 0.0, 1.0) };
const vec3 light_cols[2] = { vec3(1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0) };

float LinearizeDepth(float depth, float near_p, float far_p)
{
    float z = depth * 2.0 - 1.0; // Back to NDC 
    return (2.0 * near_p * far_p) / (far_p + near_p - z * (far_p - near_p));
}

vec4 position_from_depth(float depth)
{
    float z = subpassLoad(inDepth).r;

    vec4 clipSpace = vec4(inUV * 2.0 - 1.0, z, 1.0);
	vec4 viewSpace = inverse(ubo.proj) * clipSpace;
	viewSpace.xyz /= viewSpace.w;

    vec4 position = inverse(ubo.view) * vec4( viewSpace.xyz, 1.0 );

    return vec4(vec3(position), 1.0);
}

void main() 
{
	if(ubo.display_mode == 0)
	{
		outFragcolor = vec4(subpassLoad(inNormal).rgb, 1.0);
	}
	else if(ubo.display_mode == 1)
	{
        float z = LinearizeDepth(subpassLoad(inDepth).r, 0.1, 10.0) / 9.9;
		outFragcolor = vec4(z, z, z,  1.0);
	}
	else if(ubo.display_mode == 2)
	{
		outFragcolor = vec4(subpassLoad(inColor).a, subpassLoad(inColor).a, subpassLoad(inColor).a, 1.0);
	}
	else if(ubo.display_mode == 3)
	{
		outFragcolor = vec4(subpassLoad(inColor).rgb, 1.0);
	}
	else
	{
        vec4 position = position_from_depth(subpassLoad(inDepth).r);;

        vec3 normal = (subpassLoad(inNormal).rgb - vec3(0.5)) / 0.5;

		if(ubo.model_stage_on > 0)
        {
            if(ubo.lighting_stage_on > 0)
            {
                vec3 light_contribution = vec3(0.0, 0.0, 0.0);

                for(int light_num = 0; light_num < 2; ++light_num)
                {
                    vec3 frag_pos = position.xyz;
                    vec3 normal_dir = normalize((mat3(ubo.model) * normal).xyz);
                    vec3 light_pos = (ubo.light * lights[light_num]).xyz;
                    vec3 light_dir = normalize(frag_pos - light_pos);

                    float ambient = ubo.ambient;
                    float diffuse = ubo.diffuse * max(0.0, dot(normal_dir, -light_dir));
            
                    float specular = 0.0;
                    if(diffuse != 0.0)
                    {
                        vec3 camera_dir = normalize(frag_pos - vec3(-2.0, 0.0, 0.0));
                        vec3 reflection_dir = normalize(reflect(light_dir, normal_dir));

                        float spec_val = pow(max(dot(reflection_dir, -camera_dir), 0.0), ubo.Ns);
                        specular = clamp(subpassLoad(inColor).a * spec_val, 0.0, 1.0);
                    }

                    light_contribution += diffuse * ubo.Kd.xyz * light_cols[light_num] + specular * ubo.Ks.xyz * light_cols[light_num];
                }

                outFragcolor = vec4(clamp(ubo.Ke.xyz + subpassLoad(inColor).rgb * (ubo.ambient + (light_contribution / 2.0)), vec3(0.0), vec3(1.0)), 1.0);
             }
             else
             {
                outFragcolor = vec4(subpassLoad(inColor).rgb, 1.0);
             }
        }
        else
        {
            outFragcolor = vec4(0.0);
        }
	}
}