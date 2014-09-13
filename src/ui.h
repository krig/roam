#pragma once

void uiInit(ml_material* m);
void uiExit();
void uiBegin();
void uiDraw(SDL_Point* viewport);
void uiText(float x, float y, uint32_t clr, const char* str, ...);
