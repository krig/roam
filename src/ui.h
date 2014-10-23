#pragma once

void ui_init(ml_material* ui, ml_material* debug);
void ui_exit(void);
void ui_tick(float dt);
void ui_draw(SDL_Point* viewport);

void ui_set_scale(float scale);
void ui_text_measure(int* w, int* h, const char* str, ...);
void ui_text(float x, float y, uint32_t clr, const char* str, ...);
void ui_rect(float x, float y, float w, float h, uint32_t clr);

void ui_debug_line(ml_vec3 p1, ml_vec3 p2, uint32_t clr);
void ui_debug_aabb(ml_vec3 center, ml_vec3 extent, uint32_t clr);
void ui_debug_block(ml_ivec3 block, uint32_t clr);
void ui_debug_point(ml_vec3 p, uint32_t clr);
void ui_debug_sphere(ml_vec3 p, float r, uint32_t clr);
void ui_draw_debug(ml_matrixstack* projection, ml_matrixstack* modelview);

void ui_console_toggle(bool enable);
bool ui_console_enabled(void);
bool ui_console_handle_event(SDL_Event* event);
void ui_add_console_line(const char* txt);
