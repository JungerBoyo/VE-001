#version 450 core

precision mediump float;

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
    // vec2 uv = vec2(gl_FragCoord.x/shadow_map_width, gl_FragCoord.y/shadow_map_height);
    // uv = 2.0 * uv - 1.0;
    // uv.x *= shadow_map_aspect_ratio;

    // if (length(uv) > 1.0) {
    //     discard;
    // }
}
