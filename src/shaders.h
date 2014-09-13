/*
 * To be included only in main.c
 */

static const char* basic_vshader = "#version 130\n"
	"uniform mat4 projmat;\n"
	"uniform mat4 modelview;\n"
	"uniform mat4 normalmat;\n"
	"in vec3 position;\n"
	"in vec4 color;\n"
	"out vec4 out_color;\n"
	"void main() {\n"
	"out_color = color;\n"
	"gl_Position = projmat * modelview * vec4(position, 1);\n"
	"}\n";

static const char* basic_fshader = "#version 130\n"
	"precision highp float;\n"
	"in vec4 out_color;\n"
	"out vec4 fragment;\n"
	"void main() {\n"
	"fragment = out_color;\n"
	"}\n";

static const char* ui_vshader = "#version 130\n"
	"uniform vec2 screensize;\n"
	"in vec2 position;\n"
	"in vec2 texcoord;\n"
	"in vec4 color;\n"
	"out vec2 out_texcoord;\n"
	"out vec4 out_color;\n"
	"void main() {\n"
	"vec2 offset = screensize/2;\n"
	"vec2 eyepos = (position - offset) / offset;\n"
	"out_texcoord = texcoord;\n"
	"out_color = color;\n"
	"gl_Position = vec4(eyepos.x, eyepos.y, 0, 1);\n"
	"}\n";

static const char* ui_fshader = "#version 130\n"
	"precision highp float;\n"
	"in vec2 out_texcoord;\n"
	"in vec4 out_color;\n"
	"out vec4 fragment;\n"
	"uniform sampler2D tex0;\n"
	"void main() {\n"
	"float a = texture(tex0, out_texcoord).r;\n"
	"fragment = out_color * vec4(1, 1, 1, a);\n"
	"}\n";
