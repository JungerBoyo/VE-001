#version 450 core

layout(location = 0) in vec3 in_position;

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

void main() {
    gl_Position = current_light_transform[0] * vec4(in_position, 1.0);
}
