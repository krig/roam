#pragma once

void uiInit(ml_material* ui, ml_material* debug);
void uiExit(void);

void uiSetScale(float scale);
void uiText(float x, float y, uint32_t clr, const char* str, ...);
void uiDraw(SDL_Point* viewport);

void uiDebugLine(ml_vec3 p1, ml_vec3 p2, uint32_t clr);
void uiDebugAABB(ml_vec3 minp, ml_vec3 maxp, uint32_t clr);
void uiDebugPoint(ml_vec3 p, uint32_t clr);
void uiDebugSphere(ml_vec3 p, float r, uint32_t clr);
void uiDrawDebug(ml_matrixstack* projection, ml_matrixstack* modelview);
