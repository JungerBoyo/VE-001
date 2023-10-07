#version 450 core

precision mediump float;

///////////////////////////////////////////
//                 INPUTS                //
///////////////////////////////////////////
layout(location = 0) in vec2 in_texcoord;
layout(location = 1) flat in int in_material_id;
layout(location = 2) flat in int in_face_id;
layout(location = 3) flat in vec3 in_normal;
layout(location = 4) in vec3 in_position;

///////////////////////////////////////////
//               GENERAL UBO             //
///////////////////////////////////////////
layout(std140, binding = 0) uniform General {
    mat4 mvp;
    vec3 camera_pos;
    vec3 camera_dir;
};

///////////////////////////////////////////
//             TEX DEFINITIONS          //
///////////////////////////////////////////
layout(binding = 0) uniform sampler2DArray u_voxel_tex_array;
layout(binding = 1) uniform sampler2DArrayShadow u_depth_tex_array;
layout(binding = 2) uniform samplerCubeArrayShadow u_depth_cubetex_array;

///////////////////////////////////////////
//                MATERIALS              //
///////////////////////////////////////////
struct MaterialParams {
    vec4 specular;
    vec3 diffuse;
    float alpha;
};
layout(std430, binding = 0) readonly buffer MaterialParamsArray {
    MaterialParams material_params[];
};

struct MaterialDescriptor {
    uint material_texture_indices[6];
    uint material_params_indices[6];
};
layout(std430, binding = 1) readonly buffer Materials {
    MaterialDescriptor materials[];
};

///////////////////////////////////////////
//                LIGHTS                 //
///////////////////////////////////////////

struct DirectionalLight {
    vec3 direction;    
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shadow_map_index;
    mat4 transform;
};

struct PointLight {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    vec3 attenuation_params;    
    float shadow_map_index;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    vec3 attenuation_params;    
    vec3 cut_off; // inner, outer, epsilon
    float shadow_map_index;
    mat4 transform;
};

layout(std140, binding = 1) uniform LightsDescriptor {
    mat4 current_light_transform[6];
    float shadow_map_width;
    float shadow_map_height;
    float shadow_map_aspect_ratio;
    float cube_shadow_map_n;
    float cube_shadow_map_f;
    uint directional_lights_size;
    uint point_lights_size;
    uint spot_lights_size;
};

layout(std430, binding = 2) readonly buffer DirectionalLights {
    DirectionalLight directional_lights[];
};
layout(std430, binding = 3) readonly buffer PointLights {
    PointLight point_lights[];
};
layout(std430, binding = 4) readonly buffer SpotLights {
    SpotLight spot_lights[];
};


vec3 directionalLightsContrib(vec3 frag_pos, vec3 view_dir, vec3 normal, vec3 material_diffuse, vec4 material_specular) {
    vec3 result = vec3(0.0);
    for(uint i = 0; i < directional_lights_size; ++i) {
        DirectionalLight light = directional_lights[i];
        
        float visibility = 1.0;
        if (light.shadow_map_index != -1.0) {
            vec4 proj_frag_pos = light.transform * vec4(frag_pos, 1.0);  
            proj_frag_pos = vec4(proj_frag_pos.xyz/proj_frag_pos.w, 1.0);
            vec4 texcoord = vec4(proj_frag_pos.xy, light.shadow_map_index, proj_frag_pos.z);
            visibility = texture(u_depth_tex_array, texcoord);
        }
        
        float diffuse_component = max(dot(normal, light.direction), 0.0);

        vec3 halfway_dir = normalize(light.direction + view_dir);
        float spec_component = pow(max(dot(normal, halfway_dir), 0.0), material_specular.w);
        
        vec3 ambient = light.ambient;// * material_diffuse;
        vec3 diffuse = light.diffuse * diffuse_component;// * material_diffuse;
        vec3 specular = light.specular * spec_component * material_specular.xyz;// * material_diffuse;

        result += (ambient + visibility * (diffuse + specular)) * material_diffuse;
    }
    return result;
}

vec3 pointLightsContrib(vec3 frag_pos, vec3 view_dir, vec3 normal, vec3 material_diffuse, vec4 material_specular) {
    vec3 result = vec3(0.0);
    for(uint i = 0; i < point_lights_size; ++i) {
        PointLight light = point_lights[i];

        float visibility = 1.0;
        if (light.shadow_map_index != -1.0) {
            vec3 frag_light_dist = (frag_pos - light.position);
            vec3 abs_frag_light_dist = abs(frag_light_dist);
            float z = max(abs_frag_light_dist.x, max(abs_frag_light_dist.y, abs_frag_light_dist.z));
            z = cube_shadow_map_n + cube_shadow_map_f/z;
            vec4 texcoord = vec4(frag_light_dist, light.shadow_map_index);
            visibility = texture(u_depth_cubetex_array, texcoord, z);
        }

        vec3 light_dir = normalize(light.position - frag_pos);
        float diffuse_component = max(dot(normal, light_dir), 0.0); 
            
        vec3 halfway_dir = normalize(light_dir + view_dir);
        float spec_component = pow(max(dot(normal, halfway_dir), 0.0), material_specular.w);

        float dist = length(light.position - frag_pos);
        float attenuation = 1.0/(
            light.attenuation_params[0] + 
            light.attenuation_params[1] * dist +
            light.attenuation_params[2] * dist * dist
        );

        vec3 ambient = light.ambient;// * material_diffuse;
        vec3 diffuse = light.diffuse * diffuse_component;// * material_diffuse;
        vec3 specular = light.specular * spec_component * material_specular.xyz;

        result += attenuation * material_diffuse * (ambient + visibility * (diffuse + specular));
    }

    return result;
}

vec3 spotLightsContrib(vec3 frag_pos, vec3 view_dir, vec3 normal, vec3 material_diffuse, vec4 material_specular) {
    vec3 result = vec3(0.0);
    for(uint i = 0; i < spot_lights_size; ++i) {
        SpotLight light = spot_lights[i];

        float visibility = 1.0; 
        if (light.shadow_map_index != -1.0) {
            vec4 proj_frag_pos = light.transform * vec4(frag_pos, 1.0);  
            proj_frag_pos = vec4(proj_frag_pos.xyz/proj_frag_pos.w, 1.0);
            vec4 texcoord = vec4(proj_frag_pos.xy, light.shadow_map_index, proj_frag_pos.z);
            visibility = texture(u_depth_tex_array, texcoord);
        }

        vec3 light_dir = normalize(light.position - frag_pos);
        float diffuse_component = max(dot(normal, light_dir), 0.0); 
            
        vec3 halfway_dir = normalize(light_dir + view_dir);
        float spec_component = pow(max(dot(normal, halfway_dir), 0.0), material_specular.w);

        float dist = length(light.position - frag_pos);
        float attenuation = 1.0/(
            light.attenuation_params[0] + 
            light.attenuation_params[1] * dist +
            light.attenuation_params[2] * dist * dist
        );

        float inner_cut_off = light.cut_off[0];
        float outer_cut_off = light.cut_off[1];
        float theta = dot(light_dir, light.direction);
        float epsilon = light.cut_off[2];
        float intensity = clamp((theta - outer_cut_off)/epsilon, 0.0, 1.0);
        
        vec3 ambient = light.ambient; //* material_diffuse;
        vec3 diffuse = light.diffuse * diffuse_component;// * material_diffuse;
        vec3 specular = light.specular * spec_component * material_specular.xyz;

        result += attenuation * material_diffuse * (ambient + intensity * visibility * (diffuse + specular));
    }

    return result;
}

///////////////////////////////////////////
//               SHADOWS                 //
///////////////////////////////////////////
// float computeDirectionalShadowFactor(mat4 light_transform, vec3 frag_pos, float ) {
//     vec4 proj_frag_pos = light_transform * vec4(frag_pos, 1.0);  
//     proj_frag_pos = vec4(proj_frag_pos.xyz/proj_frag_pos.w, 1.0);
//     vec4 texcoord = vec4(proj_frag_pos.xy, float(i), proj_frag_pos.z);
//     float visibility = texture(u_depth_tex_array, texcoord);
//     return visibility;
// }

// float computeOmnidirectionalShadowFactor(vec3 frag_pos) {
//     float result = 0.0; 
//     for (uint i = 0; i < omnidirectional_shadow_casting_lights_count; i += 6) {
//         for (uint face = 0; face < 6; ++face) {
//             mat4 light_transform = omnidirectional_light_trasnforms[i];
//             vec4 proj_frag_pos = light_transform * vec4(frag_pos, 1.0);
//             proj_frag_pos = vec4(proj_frag_pos.xyz/proj_frag_pos.w, 1.0);
//             vec4 texcoord = vec4(proj_frag_pos.xy, float(face), float(i));
//             float visibility = texture(u_depth_cubetex_array, texcoord, proj_frag_pos.z);
//             result += visibility;
//         }
//     }
//     return result;
// }

///////////////////////////////////////////
//                 MAIN                  //
///////////////////////////////////////////
layout(location = 0) out vec4 out_fragment;

void main() {
    MaterialDescriptor descriptor = materials[in_material_id];

    float tex_id = float(descriptor.material_texture_indices[in_face_id]);
    MaterialParams params = material_params[descriptor.material_params_indices[in_face_id]];

    vec4 tex = texture(u_voxel_tex_array, vec3(in_texcoord, tex_id));

    // float shadow_factor = 
    //     computeDirectionalShadowFactor(in_position) +
    //     computeOmnidirectionalShadowFactor(in_position);

    vec3 result = 
        directionalLightsContrib(in_position, (camera_pos - in_position), in_normal, tex.xyz * params.diffuse, params.specular) + 
        pointLightsContrib(in_position, (camera_pos - in_position), in_normal, tex.xyz * params.diffuse, params.specular) +
        spotLightsContrib(in_position, (camera_pos - in_position), in_normal, tex.xyz * params.diffuse, params.specular);

    out_fragment = vec4(result, tex.w * params.alpha);
}