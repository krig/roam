#pragma once

void ui_init(material_t* ui, material_t* debug);
void ui_exit(void);
void ui_tick(float dt);
void ui_draw(SDL_Point* viewport);

void ui_set_scale(float scale);
void ui_text_measure(int* w, int* h, const char* str, ...);
void ui_text(float x, float y, uint32_t clr, const char* str, ...);
void ui_rect(float x, float y, float w, float h, uint32_t clr);

void ui_debug_line(vec3_t p1, vec3_t p2, uint32_t clr);
void ui_debug_aabb(vec3_t center, vec3_t extent, uint32_t clr);
void ui_debug_block(ivec3_t block, uint32_t clr);
void ui_debug_point(vec3_t p, uint32_t clr);
void ui_debug_sphere(vec3_t p, float r, uint32_t clr);
void ui_draw_debug(mtxstack_t* projection, mtxstack_t* modelview);

void ui_console_toggle(bool enable);
bool ui_console_enabled(void);
bool ui_console_handle_event(SDL_Event* event);
void ui_add_console_line(const char* txt);
