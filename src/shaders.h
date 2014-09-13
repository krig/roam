/*
 * To be included only in main.c
 */

static const char* basic_vshader = "#version 130\n"
	"uniform mat4 projmat;\n"
	"uniform mat4 modelview;\n"
	"uniform mat3 normalmat;\n"
	"in vec3 position;\n"
	"in vec3 normal;\n"
	"in vec4 color;\n"
	"out vec3 out_normal;\n"
	"out vec4 out_color;\n"
	"out float out_depth;\n"
	"void main() {\n"
	"    out_color = color;\n"
	"    out_depth = length((modelview * vec4(position, 1)).xyz);\n"
	"    out_normal = normalmat * normal;\n"
	"    gl_Position = projmat * modelview * vec4(position, 1);\n"
	"}\n";

static const char* basic_fshader = "#version 130\n"
	"precision highp float;\n"
	"uniform vec4 amb_color;\n" // w is intensity
	"uniform vec4 fog_color;\n" // w is density (0.015 ?)
	"uniform vec3 light_dir;\n"
	"in vec3 out_normal;\n"
	"in vec4 out_color;\n"
	"in float out_depth;\n"
	"out vec4 fragment;\n"
	"vec3 fog(vec3 color, vec3 fcolor, float depth, float density){\n"
	"    const float e = 2.71828182845904523536028747135266249;\n"
	"    float f = pow(e, -pow(depth*density, 2));\n"
	"    return mix(fcolor, color, f);\n"
	"}\n"
	"void main() {\n"
	"    float intensity = max(0.0, dot(normalize(out_normal), normalize(light_dir)));\n"
//	"    float amb_intensity = max(amb_color.w, 1.0 - intensity);\n"
	"    float amb_intensity = 0.2;\n"
	"    vec3 basecolor = out_color.xyz * intensity + amb_color.xyz * amb_intensity;\n"
	"    vec3 fogged = fog(basecolor, fog_color.xyz, out_depth, fog_color.w);\n"
	"    fragment = vec4(fogged, 1);\n"
	"}\n";

static const char* ui_vshader = "#version 130\n"
	"uniform vec2 screensize;\n"
	"in vec2 position;\n"
	"in vec2 texcoord;\n"
	"in vec4 color;\n"
	"out vec2 out_texcoord;\n"
	"out vec4 out_color;\n"
	"void main() {\n"
	"    vec2 offset = screensize/2;\n"
	"    vec2 eyepos = (position - offset) / offset;\n"
	"    out_texcoord = texcoord;\n"
	"    out_color = color;\n"
	"    gl_Position = vec4(eyepos.x, eyepos.y, 0, 1);\n"
	"}\n";

static const char* ui_fshader = "#version 130\n"
	"precision highp float;\n"
	"in vec2 out_texcoord;\n"
	"in vec4 out_color;\n"
	"out vec4 fragment;\n"
	"uniform sampler2D tex0;\n"
	"void main() {\n"
	"    float a = texture(tex0, out_texcoord).r;\n"
	"    fragment = out_color * vec4(1, 1, 1, a);\n"
	"}\n";
