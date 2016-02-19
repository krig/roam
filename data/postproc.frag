#version 330
uniform sampler2D fbo_texture;
in vec2 f_texcoord;
layout(location = 0) out vec4 fragment;


void main(void) {
	vec4 tex = texture(fbo_texture, f_texcoord);
	fragment = tex;
}
