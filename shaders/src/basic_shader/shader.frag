#version 450 core

precision mediump float;

layout(location = 0) in vec2 in_tex_coord;
layout(location = 1) flat in float in_face_layer;

layout(binding = 1) uniform sampler2DArray u_voxel_tex_array;

layout(location = 0) out vec4 out_fragment;

void main() {
  out_fragment = texture(u_voxel_tex_array, vec3(in_tex_coord, in_face_layer));
}