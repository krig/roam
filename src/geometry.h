#pragma once

void makeSphere(ml_mesh* mesh, float radius, int subdivisions);
void makeHemisphere(ml_mesh* mesh, float radius, int subdivisions);
void makeCube(ml_mesh* mesh, ml_vec3 size, uint32_t top_clr, uint32_t bottom_clr);
