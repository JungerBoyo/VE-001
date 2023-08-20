#version 450 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_tex_coord;

layout(std140, binding = 0) uniform TRANSFORM {
  mat4 mvp;
};

layout(location = 0) out vec2 out_tex_coord;
layout(location = 1) flat out float out_face_layer;

void main() {
  out_tex_coord = in_tex_coord.xy;
  out_face_layer = in_tex_coord.z;

  gl_Position = mvp * vec4(in_position, 1.0);
}
