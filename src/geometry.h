#pragma once

void make_sphere(mesh_t* mesh, float radius, int subdivisions);
void make_hemisphere(mesh_t* mesh, float radius, int subdivisions);
void make_cube(mesh_t* mesh, vec3_t size, uint32_t top_clr, uint32_t bottom_clr);
