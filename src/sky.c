#include "common.h"
#include "math3d.h"
#include "game.h"
#include "sky.h"
#include "geometry.h"

static mesh_t mesh;
static renderobj_t renderobj;


void sky_init()
{
	material_t* material = game.materials + MAT_SKY;
	make_hemisphere(&mesh, 5.f, 4);
	m_create_renderobj(&renderobj, material, &mesh);
	sky_tick(0);
}


void sky_exit()
{
	m_destroy_mesh(&mesh);
	m_destroy_renderobj(&renderobj);
}


void sky_draw()
{
	material_t* material = game.materials + MAT_SKY;
	glCullFace(GL_FRONT);
	glDepthFunc(GL_EQUAL);
	glDepthRange(1, 1);
	mat44_t skyview;
	vec3_t origo = {0, 0, 0};
	if (game.camera.mode != CAMERA_3RDPERSON)
		m_fpsmatrix(&skyview, origo, game.camera.pitch, game.camera.yaw);
	else {
		vec3_t at = m_dvec3tovec3(m_dvec3sub(game.player.pos, game.camera.pos));
		m_lookat(&skyview, m_vec3(0, 0, 0), at, m_up);
	}
	m_draw_begin(&renderobj);
	m_uniform_mat44(material->projmat, m_getmatrix(&game.projection));
	m_uniform_mat44(material->modelview, &skyview);
	m_uniform_vec3(material->sun_dir, &game.light_dir);
	m_uniform_vec3(material->sun_color, &game.sun_color);
	m_uniform_vec3(material->sky_dark, &game.sky_dark);
	m_uniform_vec3(material->sky_light, &game.sky_light);
	m_draw_end(&renderobj);
	glDepthFunc(GL_LESS);
	glDepthRange(0, 1);
	glCullFace(GL_BACK);
}

static
vec3_t sun_mix(const vec3_t* colors, double day_amt, double dusk_amt, double night_amt, double dawn_amt)
{
	vec3_t c;
	c.x = colors[0].x*day_amt + colors[1].x*dusk_amt + colors[2].x*night_amt + colors[3].x*dawn_amt;
	c.y = colors[0].y*day_amt + colors[1].y*dusk_amt + colors[2].y*night_amt + colors[3].y*dawn_amt;
	c.z = colors[0].z*day_amt + colors[1].z*dusk_amt + colors[2].z*night_amt + colors[3].z*dawn_amt;
	return c;
}

static inline
vec3_t mkrgb(uint32_t rgb)
{
	vec3_t c = {((float)((rgb>>16)&0xff)/255.f),
	             ((float)((rgb>>8)&0xff)/255.f),
	             ((float)((rgb)&0xff)/255.f) };
	return c;
}

void sky_tick(float dt)
{
	double daylength = DAY_LENGTH;
	if (game.fast_day_mode)
		daylength = 10.0;
	double step = (dt / daylength);
	game.time_of_day += step;
	while (game.time_of_day >= 1.0) {
		game.day += 1;
		game.time_of_day -= 1.0;
	}

	double t = game.time_of_day;
	double day_amt, night_amt, dawn_amt, dusk_amt;

	const double day_length = 0.5;
	const double dawn_length = 0.15;
	const double dusk_length = 0.1;
	const double night_length = 0.25;

	if (t >= 0 && t < day_length) {
		day_amt = 1.0;
		night_amt = dawn_amt = dusk_amt = 0;
	} else if (t >= day_length && t < (day_length + dusk_length)) {
		double f = (t - day_length) * (1.0 / dusk_length); // 0-1
		dusk_amt = sin(f * ML_PI);
		if (f < 0.5) {
			day_amt = 1.0 - dusk_amt;
			night_amt = 0.0;
		} else {
			day_amt = 0.0;
			night_amt = 1.0 - dusk_amt;
		}
		dawn_amt = 0;
	} else if (t >= (day_length + dusk_length) && t < (day_length + dusk_length + night_length)) {
		night_amt = 1.0;
		dawn_amt = dusk_amt = day_amt = 0;
	} else {
		double f = (t - (day_length + dusk_length + night_length)) * (1.0 / dawn_length); // 0-1
		dawn_amt = sin(f * ML_PI);
		if (f < 0.5) {
			night_amt = 1.0 - dawn_amt;
			day_amt = 0.0;
		} else {
			night_amt = 0.0;
			day_amt = 1.0 - dawn_amt;
		}
		dusk_amt = 0;
	}

	//double mag = 1.0 / sqrt(day_amt*day_amt + night_amt*night_amt + dawn_amt*dawn_amt + dusk_amt*dusk_amt);
	//day_amt *= mag;
	//night_amt *= mag;
	//dawn_amt *= mag;
	//dusk_amt *= mag;

	double low_light = 0.1;
	double lightlevel = ML_MAX(day_amt, low_light);
	game.light_level = lightlevel;

#define MKRGB(rgb) mkrgb(0x##rgb)

	// day, dusk, night, dawn
	const vec3_t ambient[4] = {
		MKRGB(ffffff),
		MKRGB(544769),
		MKRGB(101010),
		MKRGB(6f2168),
	};
	const vec3_t sky_dark[4] = {
		MKRGB(3F6CB4),
		MKRGB(40538e),
		MKRGB(000000),
		MKRGB(3d2163),
	};
	const vec3_t sky_light[4] = {
		MKRGB(00AAFF),
		MKRGB(6a6ca5),
		MKRGB(171b33),
		MKRGB(e16e7a),
	};
	const vec3_t sun_color[4] = {
		MKRGB(E8EAE7),
		MKRGB(fdf2c9),
		MKRGB(e2f3fa),
		MKRGB(fefebb),
	};
	const vec3_t fog[4] = {
		MKRGB(7ed4ff),
		MKRGB(ad6369),
		MKRGB(383e60),
		MKRGB(f7847a),
	};

	const float fogdensity[4] = {
		0.004,
		0.004,
		0.002,
		0.006
	};

	game.amb_light = sun_mix(ambient, day_amt, dusk_amt, night_amt, dawn_amt);
	game.sky_dark = sun_mix(sky_dark, day_amt, dusk_amt, night_amt, dawn_amt);
	game.sky_light = sun_mix(sky_light, day_amt, dusk_amt, night_amt, dawn_amt);
	game.sun_color = sun_mix(sun_color, day_amt, dusk_amt, night_amt, dawn_amt);
	vec3_t fogc = sun_mix(fog, day_amt, dusk_amt, night_amt, dawn_amt);
	float fogd = fogdensity[0]*day_amt + fogdensity[1]*dusk_amt + fogdensity[2]*night_amt + fogdensity[3]*dawn_amt;

	m_setvec4(game.fog_color, fogc.x, fogc.y, fogc.z, fogd);
	m_setvec3(game.light_dir, cos(t * ML_TWO_PI), -sin(t * ML_TWO_PI), 0);
	game.light_dir = m_vec3normalize(game.light_dir);
}
