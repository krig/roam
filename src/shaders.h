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
	"uniform vec4 amb_color;\n"
	"uniform vec4 fog_color;\n"
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
	"    float amb_intensity = min(amb_color.w, 1.0 - intensity);\n"
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


static const char* debug_vshader = "#version 130\n"
	"uniform mat4 projmat;\n"
	"uniform mat4 modelview;\n"
	"in vec3 position;\n"
	"in vec4 color;\n"
	"out vec4 out_color;\n"
	"void main() {\n"
	"    out_color = color;\n"
	"    gl_Position = projmat * modelview * vec4(position, 1);\n"
	"}\n";

static const char* debug_fshader = "#version 130\n"
	"precision highp float;\n"
	"in vec4 out_color;\n"
	"out vec4 fragment;\n"
	"void main() {\n"
	"    fragment = out_color;\n"
	"}\n";

static const char* chunk_vshader = "#version 130\n"
	"uniform mat4 projmat;\n"
	"uniform mat4 modelview;\n"
	"uniform mat3 normalmat;\n"
	"uniform vec3 chunk_offset;\n"
	"in vec4 position;\n"
	"in vec4 normal;\n"
	"in vec2 tc;\n"
	"in vec4 clr;\n"
	"out vec3 out_normal;\n"
	"out vec2 out_tc;\n"
	"out vec4 out_color;\n"
	"out float out_depth;\n"
	"void main() {\n"
	"    vec3 pos = chunk_offset.xyz * 16.0 + position.xyz * 17.0;\n"
	"    out_color = mix(vec4(0.4823529411764706, 0.5725490196078431, 0.0392156862745098, 1), vec4(0.6784313725490196, 0.8196078431372549, 0.0, 1), abs(pos.y / 16.0));\n"
	"    out_depth = length((modelview * vec4(pos.xyz, 1)).xyz);\n"
	"    out_normal = normalmat * (normal.xyz * 2.0 - 1.0);\n"
	"    out_tc = tc;\n"
	"    gl_Position = projmat * modelview * vec4(pos.xyz, 1);\n"
	"}\n";

static const char* chunk_fshader = "#version 130\n"
	"precision highp float;\n"
	"uniform vec4 amb_color;\n"
	"uniform vec4 fog_color;\n"
	"uniform vec3 light_dir;\n"
	"uniform sampler2D tex0;\n"
	"in vec3 out_normal;\n"
	"in vec2 out_tc;\n"
	"in vec4 out_color;\n"
	"in float out_depth;\n"
	"out vec4 fragment;\n"
	"vec3 fog(vec3 color, vec3 fcolor, float depth, float density){\n"
	"    const float e = 2.71828182845904523536028747135266249;\n"
	"    float f = pow(e, -pow(depth*density, 2));\n"
	"    return mix(fcolor, color, f);\n"
	"}\n"
	// out_color.xyz = torchlight level
	// out_color.w = sunlight level
	"void main() {\n"
	"    vec3 base = texture(tex0, out_tc).xyz;\n"
	"    if (base.xyz == vec3(1, 0, 1)) discard;\n"
	"    float intensity = max(0.0, dot(normalize(out_normal), normalize(light_dir))) * out_color.w;\n"
	"    vec3 light = clamp(out_color.xyz + (out_color.w * intensity), 0.0, 1.0);"
	"    base *= light.xyz;\n"
	"    vec3 fogged = fog(base, fog_color.xyz, out_depth, fog_color.w);\n"
	"    fragment = vec4(fogged.rgb, 1);\n"
	"}\n";
