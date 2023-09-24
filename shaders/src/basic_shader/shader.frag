#version 450 core

precision mediump float;

layout(location = 0) in vec2 in_texcoord;
layout(location = 1) flat in int in_material_id;
layout(location = 2) flat in int in_face_id;
layout(location = 3) flat in vec3 in_normal;
layout(location = 4) in vec3 in_position;
layout(location = 5) in vec4 in_light_space_position;

layout(std140, binding = 0) uniform General {
    mat4 mvp;
    mat4 light_mvp;
    mat4 light_mvp_biased;
    vec3 camera_pos;
    vec3 camera_dir;
    vec3 light_pos;
    vec3 light_dir;
};

layout(binding = 1) uniform sampler2DArray u_voxel_tex_array;
layout(binding = 5) uniform sampler2DShadow u_depth_tex;

struct MaterialParams {
    vec4 color;
    vec3 diffuse;
    float shininess;
    vec3 specular;
};
layout(std430, binding = 2) readonly buffer MaterialParamsArray {
    MaterialParams material_params[];
};


struct MaterialDescriptor {
    uint material_texture_indices[6];
    uint material_params_indices[6];
};
layout(std430, binding = 3) readonly buffer Materials {
    MaterialDescriptor materials[];
};

layout(location = 0) out vec4 out_fragment;

// const vec3 LIGHT_POS = vec3(-10.0, 0.0, -10.0); //vec3(64.0, 320.0, -80.0);
const vec3 LIGHT_COLOR = vec3(1.0, 1.0, 1.0);
const vec3 AMBIENT = vec3(0.1, 0.1, 0.1);

void main() {

    MaterialDescriptor descriptor = materials[in_material_id];

    float tex_id = float(descriptor.material_texture_indices[in_face_id]);
    MaterialParams params = material_params[descriptor.material_params_indices[in_face_id]];

    vec4 tex = texture(u_voxel_tex_array, vec3(in_texcoord, tex_id));

// compute light
    // diffuse
    vec3 _light_dir = normalize(light_pos - in_position);
    float diff = max(dot(in_normal, _light_dir), 0.0);
    vec3 diffuse = LIGHT_COLOR * (diff * params.diffuse);

    // specular
    vec3 view_dir = normalize(camera_pos - in_position);
    // vec3 reflect_dir = reflect(-light_dir, in_normal);
    vec3 halfway_dir = normalize(_light_dir + view_dir);
    // float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 128.0 * params.shininess);
    float spec = pow(max(dot(in_normal, halfway_dir), 0.0), params.shininess);
    vec3 specular = LIGHT_COLOR * (spec * params.specular);

// shadow
    float f = textureProj(u_depth_tex, in_light_space_position);

    vec4 result = vec4((AMBIENT + f * (diffuse + specular)) * vec3(tex), tex.w);

    // result.g = f;
    // vec4 result = vec4((AMBIENT * vec3(tex)) + (diffuse * vec3(tex)) + (specular * vec3(tex)), 1.0);

    out_fragment = result; 
}