#version 450 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_texcoord;

layout(std140, binding = 0) uniform General {
    mat4 vp;
    vec3 camera_pos;
};

layout(location = 0) flat out int out_color_id;
layout(location = 1) flat out vec3 out_normal;
layout(location = 2) out vec3 out_position;

void main() {
  
    int face = int(in_texcoord.z) % 6;
    vec3 normal = vec3(0.0);
    normal[face / 2] = 1.0 + (float(face % 2) * -2.0);
    out_normal = normal;

    out_color_id = (int(in_texcoord.z) / 6) % 9;

    out_position = in_position;
    
    gl_Position = vp * vec4(in_position, 1.0);
}
