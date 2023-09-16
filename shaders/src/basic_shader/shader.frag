#version 450 core

precision mediump float;

layout(location = 0) in vec2 in_texcoord;
layout(location = 1) flat in int in_material_id;
layout(location = 2) flat in int in_face_id;
layout(location = 3) flat in vec3 in_normal;
layout(location = 4) in vec3 in_frag_pos;

layout(std140, binding = 0) uniform General {
    mat4 mvp;
    vec3 camera_pos;
    vec3 camera_dir;
};

layout(binding = 1) uniform sampler2DArray u_voxel_tex_array;

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

const vec3 LIGHT_POS = vec3(64.0, 320.0, 0.0);
const vec3 LIGHT_COLOR = vec3(1.0, 1.0, 1.0);
const vec3 AMBIENT = vec3(0.5, 0.5, 0.5);

void main() {

    MaterialDescriptor descriptor = materials[in_material_id];

    float tex_id = float(descriptor.material_texture_indices[in_face_id]);
    MaterialParams params = material_params[descriptor.material_params_indices[in_face_id]];

    vec4 tex = texture(u_voxel_tex_array, vec3(in_texcoord, tex_id));

// compute light
    // diffuse
    vec3 light_dir = normalize(LIGHT_POS - in_frag_pos);
    float diff = max(dot(in_normal, light_dir), 0.0);
    vec3 diffuse = LIGHT_COLOR * (diff * params.diffuse);

    // specular
    vec3 view_dir = normalize(camera_pos - in_frag_pos);
    vec3 reflect_dir = reflect(-light_dir, in_normal);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), params.shininess);
    vec3 specular = LIGHT_COLOR * (spec * params.specular);

    vec4 result = vec4(AMBIENT + diffuse + specular, 1.0);

    // vec4 result = vec4((AMBIENT * vec3(tex)) + (diffuse * vec3(tex)) + (specular * vec3(tex)), 1.0);

//
    out_fragment = result; 
}