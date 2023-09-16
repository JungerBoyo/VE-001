#version 450 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_texcoord;

layout(std140, binding = 0) uniform General {
    mat4 mvp;
    vec3 camera_pos;
    vec3 camera_dir;
};

layout(location = 0) out vec2 out_texcoord;
layout(location = 1) flat out int out_material_id;
layout(location = 2) flat out int out_face_id;
layout(location = 3) flat out vec3 out_normal;
layout(location = 4) out vec3 out_frag_pos;

void main() {
    out_texcoord = in_texcoord.xy;

    int face = int(in_texcoord.z) % 6;
    out_face_id = face;
    
    vec3 normal = vec3(0.0);
    normal[face / 3] = 1.0 + (float(face % 2 == 1) * -2.0);
    out_normal = normal;

    out_material_id = int(in_texcoord.z) / 6;

    out_frag_pos = in_position;

    gl_Position = mvp * vec4(in_position, 1.0);
}
