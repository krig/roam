#version 330
uniform sampler2D fbo_texture;
in vec2 f_texcoord;
layout(location = 0) out vec4 fragment;

void main(void) {

	vec4 tex = texture(fbo_texture, f_texcoord);
	vec4 tex2 = texture(fbo_texture, vec2(0.01, 0.01) + f_texcoord);
	fragment = vec4((tex.xyz + tex2.xyz) * 0.5, 1);
}
