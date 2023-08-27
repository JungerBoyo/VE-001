#version 450 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_texcoord;

layout(std140, binding = 0) uniform TRANSFORM {
  mat4 mvp;
};

layout(location = 0) out vec2 out_texcoord;
layout(location = 1) flat out float out_face_layer;

void main() {
  out_texcoord = in_texcoord.xy;
  out_face_layer = in_texcoord.z;

  vec3 normal = vec3(0.0);
  normal[int(in_texcoord.z) / 3] = 1 + (int(int(in_texcoord.z) % 2 == 1) * -2);

  gl_Position = mvp * vec4(in_position, 1.0);
}
