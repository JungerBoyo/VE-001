#version 450 core

precision mediump float;

layout(location = 0) flat in int in_color_id;
layout(location = 1) flat in vec3 in_normal;
layout(location = 2) in vec3 in_position;

layout(location = 0) out vec4 out_fragment;

layout(std140, binding = 0) uniform General {
    mat4 vp;
    vec3 camera_pos;
};

const vec3 color_palette[9] = {
    vec3(0.0, 0.0, 0.5),
    vec3(0.0, 0.0, 1.0),
    vec3(0.0, 0.5, 1.0),
    vec3(0.0, 1.0, 1.0),
    vec3(0.5, 1.0, 0.5),
    vec3(1.0, 1.0, 0.0),
    vec3(1.0, 0.5, 0.0),
    vec3(1.0, 0.0, 0.0),
    vec3(0.5, 0.0, 0.0),
};

const vec3 LIGHT_DIR = vec3(0.5, -0.5, 0.5);
const vec3 AMBIENT = vec3(0.33, 0.3, 0.3);

void main() {
    vec3 color = color_palette[in_color_id];

    float diff = max(dot(in_normal, LIGHT_DIR), 0.0);
    vec3 diffuse = vec3(1.0, 0.9, 0.9) * (diff * color);

    vec3 view_dir = normalize(camera_pos - in_position);
    vec3 halfway_dir = normalize(LIGHT_DIR + view_dir);

    float spec = pow(max(dot(in_normal, halfway_dir), 0.0), 32.0);
    vec3 specular = vec3(1.0, 0.9, 0.9) * (spec * vec3(1.0, 0.9, 0.9));

    out_fragment = vec4((AMBIENT * color + diffuse + specular), 1.0);
}
