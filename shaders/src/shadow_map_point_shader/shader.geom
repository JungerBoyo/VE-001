#version 450 core

layout (triangles) in;
layout (triangle_strip, max_vertices=18) out;

layout(std140, binding = 1) uniform LightsDescriptor {
    mat4 current_light_transform[6];
    float shadow_map_width;
    float shadow_map_height;
    float shadow_map_aspect_ratio;
    uint directional_lights_size;
    uint point_lights_size;
    uint spot_lights_size;
};

void main() {
    for (int face = 0; face < 6; ++face) {
        gl_Layer = face;
        
        gl_Position = current_light_transform[face] * gl_in[0].gl_Position;
        EmitVertex();
        gl_Position = current_light_transform[face] * gl_in[1].gl_Position;
        EmitVertex();
        gl_Position = current_light_transform[face] * gl_in[2].gl_Position;
        EmitVertex();

        EndPrimitive();
    }
}