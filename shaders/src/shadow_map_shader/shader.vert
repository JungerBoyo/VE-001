#version 450 core

layout(location = 0) in vec3 in_position;

layout(std140, binding = 0) uniform General {
    mat4 mvp;
    mat4 light_mvp;
    mat4 light_mvp_biased;
    vec3 camera_pos;
    vec3 camera_dir;
    vec3 light_pos;
    vec3 light_dir;
};

void main() {
    gl_Position = light_mvp * vec4(in_position, 1.0);
}
