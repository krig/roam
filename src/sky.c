#include "common.h"
#include "math3d.h"
#include "game.h"
#include "sky.h"
#include "geometry.h"

static ml_mesh sky_mesh;
static ml_renderable sky_renderable;

void skyInit() {
	ml_material* material = game.materials + MAT_SKY;
	makeHemisphere(&sky_mesh, 5.f, 4);
	mlCreateRenderable(&sky_renderable, material, &sky_mesh);
}

void skyExit(void) {
	mlDestroyMesh(&sky_mesh);
	mlDestroyRenderable(&sky_renderable);
}

void skyDraw() {
	ml_material* material = game.materials + MAT_SKY;
	glCullFace(GL_FRONT);
	glDepthFunc(GL_EQUAL);
	glDepthRange(1, 1);
	ml_matrix skyview;
	ml_vec3 origo = {0, 0, 0};
	if (game.camera.mode != CAMERA_3RDPERSON) {
		mlFPSMatrix(&skyview, origo, game.camera.pitch, game.camera.yaw);
	} else {
		mlLookAt(&skyview, 0, 0, 0,
		         game.player.pos.x - game.camera.pos.x,
		         game.player.pos.y - game.camera.pos.y,
		         game.player.pos.z - game.camera.pos.z,
		         0, 1.f, 0);
	}
	mlDrawBegin(&sky_renderable);
	mlUniformMatrix(material->projmat, mlGetMatrix(&game.projection));
	mlUniformMatrix(material->modelview, &skyview);
	mlUniformVec3(material->sun_dir, &game.light_dir);
	mlUniformVec3(material->sun_color, &game.sun_color);
	mlUniformVec3(material->sky_dark, &game.sky_dark);
	mlUniformVec3(material->sky_light, &game.sky_light);
	mlDrawEnd(&sky_renderable);
	glDepthFunc(GL_LESS);
	glDepthRange(0, 1);
	glCullFace(GL_BACK);
}

