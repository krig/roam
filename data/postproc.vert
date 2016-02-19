#version 330
layout(location = 0) in vec2 v_coord;
out vec2 f_texcoord;

void main(void) {
  f_texcoord = (v_coord + 1.0) / 2.0;
  gl_Position = vec4(v_coord, 0.0, 1.0);
}
