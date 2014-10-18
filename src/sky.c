#include "common.h"
#include "math3d.h"
#include "game.h"
#include "sky.h"
#include "geometry.h"

static ml_mesh sky_mesh;
static ml_renderable sky_renderable;

void skyInit() {
	ml_material* material = game.materials + MAT_SKY;
	makeSphere(&sky_mesh, 5.f, 2);
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
	mlFPSMatrix(&skyview, origo, game.camera.pitch, game.camera.yaw);
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

