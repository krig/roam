#include "common.h"
#include "math3d.h"
#include "game.h"
#include "map.h"
#include "noise.h"
#include "images.h"
#include "blocks.h"
#include "ui.h"
#include "gen.h"
#include "easing.h"

static inline
tc2us_t make_tc2us(vec2_t tc)
{
	tc2us_t to = {
		(uint16_t)(tc.x * (double)0xffff),
		(uint16_t)(tc.y * (double)0xffff)
	};
	return to;
}

uint32_t block_at(int x, int y, int z)
{
	if (y < 0 || y >= MAP_BLOCK_HEIGHT)
		return (0xf0000000|BLOCK_AIR);
	size_t idx = block_index(x, y, z);
	if (idx >= MAP_BUFFER_SIZE)
		return (0xf0000000|BLOCK_AIR);
	return map_blocks[idx];
}

void chunk_mark_dirty_ptr(game_chunk* chunk);
void chunk_destroy_mesh_ptr(game_chunk* chunk);

/*
  Set up a lookup table used for the texcoords of all regular blocks.
  Things with different dimensions need a different system..
 */
static tc2us_t block_texcoords[NUM_BLOCKTYPES * 6 * 4];
#define BLOCKTC(t, f, i) block_texcoords[(t)*(6*4) + (f)*4 + (i)]
static
void gen_block_tcs()
{
	float bw = IMG_TCW - IMG_TC_BIAS*2.0;
	vec2_t tcoffs[4] = {{0, bw}, {bw, bw}, {bw, 0}, {0, 0} };
	for (int t = 0; t < NUM_BLOCKTYPES; ++t) {
		for (int i = 0; i < 6; ++i) {
			vec2_t tl = m_vec2addf(imgTC(blockinfo[t].img[i]), IMG_TC_BIAS);
			for (int j = 0; j < 4; ++j)
				BLOCKTC(t, i, j) = make_tc2us(m_vec2add(tl, tcoffs[j]));
		}
	}
}

extern tex2d_t blocks_texture;
uint32_t* map_blocks = NULL;
static chunkpos_t map_chunk;

static uint32_t lightlut[256];

static
void lightlut_init(void)
{
	double base_level = 1.0;
	for (int i = 0; i < 256; ++i) {
		lightlut[i] = (uint32_t)base_level + (uint32_t)trunc(((double)i / 255.0)*(255.0 - base_level));
		lightlut[i] = ML_MIN(255, lightlut[i]);
		printf("%02x ", lightlut[i]);
	}
	printf("\n");
}

void map_init()
{
	blocks_init();
	gen_block_tcs();
	lightlut_init();

	printf("* Allocate and build initial map...\n");
	memset(&game.map, 0, sizeof(struct game_map));
	map_blocks = (uint32_t*)malloc(sizeof(uint32_t)*MAP_BUFFER_SIZE);
	memset(map_blocks, 0, sizeof(uint32_t)*MAP_BUFFER_SIZE);

	game.map.seed = sys_urandom();
	printf("* Seed: %lx\n", game.map.seed);
	simplex_init(game.map.seed);
	opensimplex_init(game.map.seed);

	chunkpos_t camera = player_chunk();
	for (int z = -VIEW_DISTANCE; z < VIEW_DISTANCE; ++z)
		for (int x = -VIEW_DISTANCE; x < VIEW_DISTANCE; ++x)
			chunk_load(camera.x + x, camera.z + z);
	map_chunk = camera;

	map_tick();
	printf("* Map load complete.\n");
}


void map_exit()
{
	for (size_t i = 0; i < MAP_CHUNK_WIDTH*MAP_CHUNK_WIDTH; ++i)
		chunk_destroy_mesh_ptr(game.map.chunks + i);

	free(map_blocks);
	map_blocks = NULL;
}

static inline game_chunk* cached_chunk_at(int x, int z)
{
	int bufx = mod(x, MAP_CHUNK_WIDTH);
	int bufz = mod(z, MAP_CHUNK_WIDTH);
	game_chunk* chunk = game.map.chunks + (bufz*MAP_CHUNK_WIDTH + bufx);
	if (chunk->x == x && chunk->z == z)
		return chunk;
	return 0;
}


void map_tick()
{
	// here is the correct chunk update algorithm:
	// pass through all cache slots. If a cache slot
	// doesn't match its loaded chunk data, that cache
	// slot plus all surrounding cache slots must be
	// reloaded.
	// for each slot:
	// if not dirty,
	// push to reload queue and mark as dirty

	chunkpos_t nc = player_chunk();
	if (nc.x != map_chunk.x || nc.z != map_chunk.z) {
		game_chunk* chunks = game.map.chunks;
		int cx = nc.x;
		int cz = nc.z;
		printf("[%d, %d] -> [%d, %d] (%g, %g)\n",
		       map_chunk.x, map_chunk.z, cx, cz,
		       game.camera.pos.x, game.camera.pos.z);
		map_chunk = nc;

		for (int dz = -VIEW_DISTANCE; dz < VIEW_DISTANCE; ++dz) {
			int bz = mod(cz + dz, MAP_CHUNK_WIDTH);
 			game_chunk* chunk_row = chunks + (bz*MAP_CHUNK_WIDTH);
			for (int dx = -VIEW_DISTANCE; dx < VIEW_DISTANCE; ++dx) {
				int bx = mod(cx + dx, MAP_CHUNK_WIDTH);
				game_chunk* chunk = chunk_row + bx;
				if (chunk->x != cx + dx ||
				    chunk->z != cz + dz) {
					chunk_mark_dirty_ptr(chunk);
					chunk_load(cx + dx, cz + dz);
					chunk = chunks + (bz*MAP_CHUNK_WIDTH + bx);
					assert(chunk->x == (cx + dx) && chunk->z == (cz + dz));

					// mark surrounding chunks dirty unless they are also
					// invalidated
					{
						game_chunk* surround;
						if ((surround = cached_chunk_at(chunk->x-1, chunk->z)) != 0)
							chunk_mark_dirty_ptr(surround);
						if ((surround = cached_chunk_at(chunk->x+1, chunk->z)) != 0)
							chunk_mark_dirty_ptr(surround);
						if ((surround = cached_chunk_at(chunk->x, chunk->z-1)) != 0)
							chunk_mark_dirty_ptr(surround);
						if ((surround = cached_chunk_at(chunk->x, chunk->z+1)) != 0)
							chunk_mark_dirty_ptr(surround);
					}
				}
			}
		}
	}
	{
		// TODO: try different meshing patterns, processing out from
		// center should look best
		Uint32 max_per_frame = 10; // milliseconds
		Uint32 start_ticks = SDL_GetTicks();
		Uint32 curr_ticks = start_ticks;
		int cx = nc.x;
		int cz = nc.z;
		game_chunk* chunks = game.map.chunks;
		for (int dz = -VIEW_DISTANCE; dz < VIEW_DISTANCE; ++dz) {
			int bz = mod(cz + dz, MAP_CHUNK_WIDTH);
			game_chunk* chunk_row = chunks + (bz*MAP_CHUNK_WIDTH);
			for (int dx = -VIEW_DISTANCE; dx < VIEW_DISTANCE; ++dx) {
				int bx = mod(cx + dx, MAP_CHUNK_WIDTH);
				game_chunk* chunk = chunk_row + bx;
				if (chunk->dirty) {
					chunk_build_mesh_ptr(bx, bz, chunk);
					curr_ticks = SDL_GetTicks();
					if (curr_ticks < start_ticks || ((curr_ticks - start_ticks) > max_per_frame))
						goto escape;
				}
			}
		}
	}
escape:
	return;
}

#define MAX_ALPHAS ((VIEW_DISTANCE*2)*(VIEW_DISTANCE*2))

struct alpha_t {
	game_chunk* chunk;
	vec3_t offset;
};

static struct alpha_t alphas[MAX_ALPHAS];
static size_t nalphas;

void map_draw(frustum_t* frustum)
{
	// for each visible chunk...
	// set up material etc. once.
	material_t* material = game.materials + MAT_CHUNK;

	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	m_tex2d_bind(&blocks_texture, 0);

	m_use(material);
	m_uniform_i(material->tex0, 0);
	m_uniform_mat44(material->projmat, m_getmatrix(&game.projection));
	m_uniform_mat44(material->modelview, m_getmatrix(&game.modelview));
	m_uniform_vec3(material->amb_light, &game.amb_light);
	m_uniform_vec4(material->fog_color, &game.fog_color);

	float chunk_radius = (float)CHUNK_SIZE*0.5f;
	vec3_t offset, center, extent;
	game_chunk* chunks = game.map.chunks;
	chunkpos_t camera = player_chunk();

	int dx, dz, bx, bz, x, z, j;
	game_chunk* chunk;
	mesh_t* mesh;

	nalphas = 0;

	for (dz = -VIEW_DISTANCE; dz < VIEW_DISTANCE; ++dz) {
		for (dx = -VIEW_DISTANCE; dx < VIEW_DISTANCE; ++dx) {
			bx = mod(camera.x + dx, MAP_CHUNK_WIDTH);
			bz = mod(camera.z + dz, MAP_CHUNK_WIDTH);
			chunk = chunks + (bz*MAP_CHUNK_WIDTH + bx);
			x = chunk->x - camera.x;
			z = chunk->z - camera.z;

			m_setvec3(offset, (float)(x*CHUNK_SIZE) - 0.5f, -0.5f, (float)(z*CHUNK_SIZE) - 0.5f);
			m_setvec3(center, offset.x + chunk_radius, MAP_BLOCK_HEIGHT*0.5f, offset.z + chunk_radius);
			m_setvec3(extent, chunk_radius, MAP_BLOCK_HEIGHT*0.5f, chunk_radius);
			if (collide_frustum_aabb_xz(frustum, center, extent) == ML_OUTSIDE)
				continue;

			if (chunk->dirty) {
				//if (game.debug_mode)
				//	ui_debug_aabb(center, extent, 0x44ff2222);
				//continue;
			}

			extent.y = chunk_radius;

			m_uniform_vec3(material->chunk_offset, &offset);

			if (chunk->alpha.vbo != 0 && nalphas < MAX_ALPHAS) {
				alphas[nalphas].chunk = chunk;
				alphas[nalphas].offset = offset;
				nalphas++;
			}

			for (j = 0; j < MAP_CHUNK_HEIGHT; ++j) {
				mesh = chunk->solid + j;
				if (mesh->vbo == 0)
					continue;
				center.y = (float)(CHUNK_SIZE*j) - 0.5f + chunk_radius;
				if (collide_frustum_aabb_y(frustum, center, extent) == ML_OUTSIDE)
					continue;
				m_draw(mesh);
			}
		}
	}

	m_use(NULL);
	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);
}

// sort back to front

static
int cmp_alpha_chunks(struct alpha_t *a, struct alpha_t *b)
{
	int ret = 0;
	float da = (a->offset.x*a->offset.x) + (a->offset.z*a->offset.z);
	float db = (b->offset.x*b->offset.x) + (b->offset.z*b->offset.z);
	if (da > db)
		ret = -1;
	if (da < db)
		ret = 1;
	return ret;
}

bool alpha_sort_chunks = true;

void map_draw_alphapass()
{
	size_t i;
	material_t* material;
	struct alpha_t* alpha;
	if (nalphas == 0)
		return;

	if (alpha_sort_chunks)
		qsort(alphas, nalphas, sizeof(struct alpha_t), (int(*)(const void*, const void*))cmp_alpha_chunks);

	material = game.materials + MAT_CHUNK_ALPHA;
	m_tex2d_bind(&blocks_texture, 0);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	m_use(material);
	m_uniform_i(material->tex0, 0);
	m_uniform_mat44(material->projmat, m_getmatrix(&game.projection));
	m_uniform_mat44(material->modelview, m_getmatrix(&game.modelview));
	m_uniform_vec3(material->amb_light, &game.amb_light);
	m_uniform_vec4(material->fog_color, &game.fog_color);

	for (i = 0, alpha = alphas; i < nalphas; ++i, ++alpha) {
		game_chunk* chunk = alpha->chunk;
		mesh_t* mesh = &(chunk->alpha);
		m_uniform_vec3(material->chunk_offset, &alpha->offset);
		m_draw(mesh);
	}

	m_use(NULL);
	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);
}

void map_update_block(ivec3_t block, uint32_t value)
{
	size_t idx = block_by_coord(block);

	// relight column down (fixes sunlight)
	uint32_t sunlight = 0xf0000000;
	map_blocks[idx] = value;
	idx -= block.y;
	for (int dy = MAP_BLOCK_HEIGHT-1; dy >= 0; --dy) {
		uint32_t t = map_blocks[idx + dy];
		if ((t & 0xff) != BLOCK_AIR)
			sunlight = 0;
		map_blocks[idx + dy] = (t & 0x0fffffff) | sunlight;
	}
	// TODO: need to re-propagate light from lightsources affected by this change

	chunkpos_t chunk = block_chunk(block);
	bool tess[4] = { false, false, false, false };
	int mx = block.x % CHUNK_SIZE;
	int mz = block.z % CHUNK_SIZE;
	if (mx == 0 || mx == -CHUNK_SIZE-1) {
		tess[0] = true;
	} else if (mx == -1 || mx == CHUNK_SIZE-1) {
		tess[1] = true;
	}
	if (mz == 0 || mz == -CHUNK_SIZE-1) {
		tess[2] = true;
	} else if (mz == -1 || mz == CHUNK_SIZE-1) {
		tess[3] = true;
	}
	printf("reload chunk [%d, %d] [%d, %d]\n", chunk.x, chunk.z, mx, mz);
	chunk_mark_dirty(chunk.x, chunk.z);
	if (tess[0]) {
		printf("reload chunk [%d, %d]\n", chunk.x-1, chunk.z);
		chunk_mark_dirty(chunk.x-1, chunk.z);
	}
	if (tess[1]) {
		printf("reload chunk [%d, %d]\n", chunk.x+1, chunk.z);
		chunk_mark_dirty(chunk.x+1, chunk.z);
	}
	if (tess[2]) {
		printf("reload chunk [%d, %d]\n", chunk.x, chunk.z-1);
		chunk_mark_dirty(chunk.x, chunk.z-1);
	}
	if (tess[3]) {
		printf("reload chunk [%d, %d]\n", chunk.x, chunk.z+1);
		chunk_mark_dirty(chunk.x, chunk.z+1);
	}
}

// TODO: chunk saving/loading
// TODO: asynchronous
// as a test, just fill in the designated chunk
// and tesselate the whole thing

void chunk_load(int x, int z) {
	int bufx = mod(x, MAP_CHUNK_WIDTH);
	int bufz = mod(z, MAP_CHUNK_WIDTH);
	game_chunk* chunk = game.map.chunks + (bufz*MAP_CHUNK_WIDTH + bufx);
	chunk->x = x;
	chunk->z = z;
	chunk_destroy_mesh_ptr(chunk);
	gen_loadchunk(chunk);
}

void chunk_destroy_mesh_ptr(game_chunk* chunk)
{
	for (int i = 0; i < MAP_CHUNK_HEIGHT; ++i)
		m_destroy_mesh(chunk->solid + i);
	m_destroy_mesh(&chunk->alpha);
	chunk->dirty = true;
}

void chunk_mark_dirty_ptr(game_chunk* chunk)
{
	chunk->dirty = true;
}

void chunk_mark_dirty(int x, int z)
{
	int bufx = mod(x, MAP_CHUNK_WIDTH);
	int bufz = mod(z, MAP_CHUNK_WIDTH);
	game_chunk* chunk = game.map.chunks + (bufz*MAP_CHUNK_WIDTH + bufx);
	if (chunk->x != x || chunk->z != z)
		return;
	chunk_mark_dirty_ptr(chunk);
}

// tesselation buffer: size is maximum number of triangles generated
//   1: fill tesselation buffer
//  2: allocate mesh
//   3: fill vertices
//   returns num verts in chunk

#define ALPHA_BUFFER_SIZE (CHUNK_SIZE*CHUNK_SIZE*CHUNK_SIZE*36)
static block_vtx_t alpha_buffer[ALPHA_BUFFER_SIZE];

#define TESSELATION_BUFFER_SIZE (CHUNK_SIZE*CHUNK_SIZE*CHUNK_SIZE*36)
static block_vtx_t tesselation_buffer[TESSELATION_BUFFER_SIZE];

static
bool mesh_subchunk(mesh_t* mesh, int bufx, int bufz, int cy, size_t* alphai);

static
vec3_t avg3(vec3_t a, vec3_t b, vec3_t c) {
	vec3_t r = {
		(a.x + b.x + c.x) * (1.f / 3.f),
		(a.y + b.y + c.y) * (1.f / 3.f),
		(a.z + b.z + c.z) * (1.f / 3.f)
	};
	return r;
}

static
int cmp_alpha_faces(block_face_t* a, block_face_t* b)
{
	vec3_t ca = avg3(a->vtx[0].pos, a->vtx[1].pos, a->vtx[2].pos);
	vec3_t cb = avg3(b->vtx[0].pos, b->vtx[1].pos, b->vtx[2].pos);
	if (ca.y < cb.y)
		return -1;
	if (ca.y > cb.y)
		return 1;
	if (ca.x < cb.x)
		return -1;
	if (ca.x > cb.x)
		return 1;
	if (ca.z < cb.z)
		return -1;
	if (ca.z > cb.z)
		return 1;
	return 0;
}

void chunk_build_mesh_ptr(int bufx, int bufz, game_chunk* chunk)
{
	if (!chunk->dirty)
		return;
	chunk_destroy_mesh_ptr(chunk);
	chunk->dirty = false;
	size_t alphai = 0;
	mesh_t* mesh = chunk->solid;
	for (int y = 0; y < MAP_CHUNK_HEIGHT; ++y)
		mesh_subchunk(mesh + y, bufx, bufz, y, &alphai);
	if (alphai > 0) {
		mesh_t* alpha = &(chunk->alpha);
		size_t nalphafaces = (alphai/3);
		qsort(alpha_buffer, nalphafaces, sizeof(block_face_t), (int(*)(const void*, const void*))cmp_alpha_faces);
		m_create_mesh(alpha, alphai, alpha_buffer, ML_POS_3F | ML_TC_2US | ML_CLR_4UB, GL_DYNAMIC_DRAW);
		m_set_material(alpha, game.materials + MAT_CHUNK_ALPHA);
	}
}

void chunk_build_mesh(int x, int z)
{
	int bufx = mod(x, MAP_CHUNK_WIDTH);
	int bufz = mod(z, MAP_CHUNK_WIDTH);
	game_chunk* chunk = game.map.chunks + bufz*MAP_CHUNK_WIDTH + bufx;
	if (chunk->x != x || chunk->z != z)
		return;
	chunk_build_mesh_ptr(bufx, bufz, chunk);
}

// Interleave lower 16 bits of x and y in groups of 4
// ie turn 0xabcd into 0xaabbccdd
static
uint32_t bitexpand16(uint32_t x)
{
	x = ((x << 12) & 0xF000000) + ((x << 8) & 0xF0000) + ((x << 4) & 0xF00) + (x & 0xF);
	return x | (x << 4);
}

// turn 0xaabbccdd into 0xabcd
static
uint32_t bitcontract16(uint32_t x)
{
	return ((x >> 16) & 0xF000) + ((x >> 12) & 0xF00) + ((x >> 8) & 0xF0) + ((x & 0xF0) >> 4);
}

// take light from the given four blocks and mix it into a single light value
// assert(avglight(0xabcd0000, 0xabcd0000, 0xabcd0000, 0xabcd0000) == 0xafbfcfdf);
// assert(avglight(0, 0, 0, 0) == 0x0f0f0f0f);
// assert(avglight(0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000) == 0xffff0000);
static
uint32_t avglight(uint32_t b0, uint32_t b1, uint32_t b2, uint32_t b3)
{
#define AVGLIGHT_SCALE(f) (lightlut[ML_MIN(255, (int)trunc(((double)(f)/60.0)*255.5))])
	static const uint32_t M[4] = { 0xf0000000, 0xf000000, 0xf00000, 0xf0000 };
	static const uint32_t S[4] = { 28, 24, 20, 16 };
	uint32_t ret = 0;
	ret |= AVGLIGHT_SCALE(((b0 & M[0]) >> S[0]) +
	                      ((b1 & M[0]) >> S[0]) +
	                      ((b2 & M[0]) >> S[0]) +
	                      ((b3 & M[0]) >> S[0])) << 24;
	ret |= AVGLIGHT_SCALE(((b0 & M[1]) >> S[1]) +
	                      ((b1 & M[1]) >> S[1]) +
	                      ((b2 & M[1]) >> S[1]) +
	                      ((b3 & M[1]) >> S[1])) << 16;
	ret |= AVGLIGHT_SCALE(((b0 & M[2]) >> S[2]) +
	                      ((b1 & M[2]) >> S[2]) +
	                      ((b2 & M[2]) >> S[2]) +
	                      ((b3 & M[2]) >> S[2])) << 8;
	ret |= AVGLIGHT_SCALE(((b0 & M[3]) >> S[3]) +
	                       ((b1 & M[3]) >> S[3]) +
	                       ((b2 & M[3]) >> S[3]) +
	                       ((b3 & M[3]) >> S[3]));
	return ret;
}

// TODO: calculate index of surrounding blocks directly without going through blocktype
#define POS(x, y, z) m_vec3(x, y, z)
#define BNONSOLID(t) (blockinfo[(n[(t)] & 0xff)].density < density)
//#define BNONSOLID(t) ((n[(t)] & 0xff) == BLOCK_AIR)
#define BLOCKAT(x, y, z) (map_blocks[block_index((x), (y), (z))])
#define BLOCKLIGHT(a, b, c, d, e, f, g) avglight(n[a], n[b], n[c], n[d])
#define GETCOL(np, ng, x, y, z) memcpy(n + (np), map_blocks + block_index(bx + (x), by + (y), bz + (z)), sizeof(uint32_t) * (ng))
#define FLIPCHECK() ((corners[0].clr>>24) + (corners[2].clr>>24) > (corners[1].clr>>24) + (corners[3].clr>>24))

// n array layout:
//+y       +y       +y
// \ 02 05 08  | 11 14 17  | 20 23 26
// \ 01 04 07  | 10 13 16  | 19 22 25
// \ 00 03 06  | 09 12 15  | 18 21 24
// +-----+x +-----+x +-----+x
//  (iz-1)   (iz)     (iz+1)


bool mesh_subchunk(mesh_t* mesh, int bufx, int bufz, int cy, size_t* alphai)
{
	int ix, iy, iz;
	int bx, by, bz;
	size_t vi;
	block_vtx_t* verts;

	verts = tesselation_buffer;
	vi = 0;
	bx = bufx*CHUNK_SIZE;
	by = cy*CHUNK_SIZE;
	bz = bufz*CHUNK_SIZE;
	size_t nprocessed = 0;

	size_t idx0;
	uint32_t t;
	uint32_t n[27]; // blocktypes for a 3x3 cube around this block
	int density;
	size_t save_vi = 0;

	// fill in verts
	for (iz = 0; iz < CHUNK_SIZE; ++iz) {
		for (ix = 0; ix < CHUNK_SIZE; ++ix) {
			idx0 = block_index(bx+ix, by, bz+iz);
			for (iy = 0; iy < CHUNK_SIZE; ++iy) {
				t = map_blocks[idx0 + iy] & 0xff;
				density = blockinfo[t].density;
				if (t == BLOCK_AIR) {
					++nprocessed;
					// in sunlight, no more blocks above
					if ((t & 0xf0000000) == 0xf)
						break;
					else
						continue;
				}

				if (blockinfo[t].flags & BLOCK_ALPHA) {
					save_vi = vi;
					verts = alpha_buffer;
					vi = *alphai;
				}

				if (by+iy+1 >= MAP_BLOCK_HEIGHT) {
					n[2] = n[5] = n[8] = n[11] = n[14] = n[17] = n[20] = n[23] = n[26] = (0xf0000000|BLOCK_AIR);
					GETCOL(0, 2, ix - 1, iy - 1, iz - 1);
					GETCOL(3, 2, ix, iy - 1, iz - 1);
					GETCOL(6, 2, ix + 1, iy - 1, iz - 1);
					GETCOL(9, 2, ix - 1, iy - 1, iz);
					GETCOL(12, 2, ix, iy - 1, iz);
					GETCOL(15, 2, ix + 1, iy - 1, iz);
					GETCOL(18, 2, ix - 1, iy - 1, iz + 1);
					GETCOL(21, 2, ix, iy - 1, iz + 1);
					GETCOL(24, 2, ix + 1, iy - 1, iz + 1);
				} else if (by+iy-1 < 0) {
					n[0] = n[3] = n[6] = n[9] = n[12] = n[15] = n[18] = n[21] = n[24] = (0xf0000000|BLOCK_AIR);
					GETCOL(1, 2, ix - 1, iy, iz - 1);
					GETCOL(4, 2, ix, iy, iz - 1);
					GETCOL(7, 2, ix + 1, iy, iz - 1);
					GETCOL(10, 2, ix - 1, iy, iz);
					GETCOL(13, 2, ix, iy, iz);
					GETCOL(16, 2, ix + 1, iy, iz);
					GETCOL(19, 2, ix - 1, iy, iz + 1);
					GETCOL(22, 2, ix, iy, iz + 1);
					GETCOL(25, 2, ix + 1, iy, iz + 1);
				} else {
					GETCOL(0, 3, ix - 1, iy - 1, iz - 1);
					GETCOL(3, 3, ix, iy - 1, iz - 1);
					GETCOL(6, 3, ix + 1, iy - 1, iz - 1);
					GETCOL(9, 3, ix - 1, iy - 1, iz);
					GETCOL(12, 3, ix, iy - 1, iz);
					GETCOL(15, 3, ix + 1, iy - 1, iz);
					GETCOL(18, 3, ix - 1, iy - 1, iz + 1);
					GETCOL(21, 3, ix, iy - 1, iz + 1);
					GETCOL(24, 3, ix + 1, iy - 1, iz + 1);
				}

				if (BNONSOLID(14)) {
					assert((n[14]&0xff) != (n[13]&0xff));
					const tc2us_t* tc = &BLOCKTC(t, BLOCK_TEX_TOP, 0);
					block_vtx_t corners[4];
					corners[0].pos = POS(  ix, by+iy+1, iz+1), corners[0].tc = tc[0], corners[0].clr = BLOCKLIGHT(20,23,11,14,10,19,22);
					corners[1].pos = POS(ix+1, by+iy+1, iz+1), corners[1].tc = tc[1], corners[1].clr = BLOCKLIGHT(23,26,14,17,16,22,25);
					corners[2].pos = POS(ix+1, by+iy+1,   iz), corners[2].tc = tc[2], corners[2].clr = BLOCKLIGHT( 5, 8,14,17,4,7,16);
					corners[3].pos = POS(  ix, by+iy+1,   iz), corners[3].tc = tc[3], corners[3].clr = BLOCKLIGHT( 2, 5,11,14,1,4,10);
					if (FLIPCHECK()) {
						verts[vi++] = corners[0];
						verts[vi++] = corners[1];
						verts[vi++] = corners[2];
						verts[vi++] = corners[2];
						verts[vi++] = corners[3];
						verts[vi++] = corners[0];
					} else {
						verts[vi++] = corners[0];
						verts[vi++] = corners[1];
						verts[vi++] = corners[3];
						verts[vi++] = corners[3];
						verts[vi++] = corners[1];
						verts[vi++] = corners[2];
					}
				}
				if (BNONSOLID(12)) {
					const tc2us_t* tc = &BLOCKTC(t, BLOCK_TEX_BOTTOM, 0);
					block_vtx_t corners[4];
					corners[0].pos = POS(  ix, by+iy,   iz), corners[0].tc = tc[0], corners[0].clr = BLOCKLIGHT( 0, 3, 9,12,1,4,10);
					corners[1].pos = POS(ix+1, by+iy,   iz), corners[1].tc = tc[1], corners[1].clr = BLOCKLIGHT( 3, 6,12,15,4,7,16);
					corners[2].pos = POS(ix+1, by+iy, iz+1), corners[2].tc = tc[2], corners[2].clr = BLOCKLIGHT(12,15,21,24,16,22,25);
					corners[3].pos = POS(  ix, by+iy, iz+1), corners[3].tc = tc[3], corners[3].clr = BLOCKLIGHT( 9,12,18,21,10,19,22);
					if (FLIPCHECK()) {
						verts[vi++] = corners[0];
						verts[vi++] = corners[1];
						verts[vi++] = corners[2];
						verts[vi++] = corners[2];
						verts[vi++] = corners[3];
						verts[vi++] = corners[0];
					} else {
						verts[vi++] = corners[0];
						verts[vi++] = corners[1];
						verts[vi++] = corners[3];
						verts[vi++] = corners[3];
						verts[vi++] = corners[1];
						verts[vi++] = corners[2];
					}
				}
				if (BNONSOLID(10)) {
					const tc2us_t* tc = &BLOCKTC(t, BLOCK_TEX_LEFT, 0);
					block_vtx_t corners[4];
					corners[0].pos = POS(ix,   by+iy,   iz), corners[0].tc = tc[0], corners[0].clr = BLOCKLIGHT( 0, 1, 9,10,3,4,12);
					corners[1].pos = POS(ix,   by+iy, iz+1), corners[1].tc = tc[1], corners[1].clr = BLOCKLIGHT( 9,10,18,19,12,21,22);
					corners[2].pos = POS(ix, by+iy+1, iz+1), corners[2].tc = tc[2], corners[2].clr = BLOCKLIGHT(10,11,19,20,14,22,23);
					corners[3].pos = POS(ix, by+iy+1,   iz), corners[3].tc = tc[3], corners[3].clr = BLOCKLIGHT( 1, 2,10,11,4,5,14);
					if (FLIPCHECK()) {
						verts[vi++] = corners[0];
						verts[vi++] = corners[1];
						verts[vi++] = corners[2];
						verts[vi++] = corners[2];
						verts[vi++] = corners[3];
						verts[vi++] = corners[0];
					} else {
						verts[vi++] = corners[0];
						verts[vi++] = corners[1];
						verts[vi++] = corners[3];
						verts[vi++] = corners[3];
						verts[vi++] = corners[1];
						verts[vi++] = corners[2];
					}
				}
				if (BNONSOLID(16)) {
					const tc2us_t* tc = &BLOCKTC(t, BLOCK_TEX_RIGHT, 0);
					block_vtx_t corners[4];
					corners[0].pos = POS(ix+1,   by+iy, iz+1), corners[0].tc = tc[0], corners[0].clr = BLOCKLIGHT(15,16,24,25,12,21,22);
					corners[1].pos = POS(ix+1,   by+iy,   iz), corners[1].tc = tc[1], corners[1].clr = BLOCKLIGHT( 6, 7,15,16,3,4,12);
					corners[2].pos = POS(ix+1, by+iy+1,   iz), corners[2].tc = tc[2], corners[2].clr = BLOCKLIGHT( 7, 8,16,17,4,5,14);
					corners[3].pos = POS(ix+1, by+iy+1, iz+1), corners[3].tc = tc[3], corners[3].clr = BLOCKLIGHT(16,17,25,26,14,22,23);
					if (FLIPCHECK()) {
						verts[vi++] = corners[0];
						verts[vi++] = corners[1];
						verts[vi++] = corners[2];
						verts[vi++] = corners[2];
						verts[vi++] = corners[3];
						verts[vi++] = corners[0];
					} else {
						verts[vi++] = corners[0];
						verts[vi++] = corners[1];
						verts[vi++] = corners[3];
						verts[vi++] = corners[3];
						verts[vi++] = corners[1];
						verts[vi++] = corners[2];
					}
				}
				if (BNONSOLID(22)) {
					tc2us_t* tc = &BLOCKTC(t, BLOCK_TEX_FRONT, 0);
					block_vtx_t corners[4];
					corners[0].pos = POS(  ix,   by+iy, iz+1), corners[0].tc = tc[0], corners[0].clr = BLOCKLIGHT(18,19,21,22,9,10,12);
					corners[1].pos = POS(ix+1,   by+iy, iz+1), corners[1].tc = tc[1], corners[1].clr = BLOCKLIGHT(21,22,24,25,12,15,16);
					corners[2].pos = POS(ix+1, by+iy+1, iz+1), corners[2].tc = tc[2], corners[2].clr = BLOCKLIGHT(22,23,25,26,14,16,17);
					corners[3].pos = POS(  ix, by+iy+1, iz+1), corners[3].tc = tc[3], corners[3].clr = BLOCKLIGHT(19,20,22,23,10,11,14);
					if (FLIPCHECK()) {
						verts[vi++] = corners[0];
						verts[vi++] = corners[1];
						verts[vi++] = corners[2];
						verts[vi++] = corners[2];
						verts[vi++] = corners[3];
						verts[vi++] = corners[0];
					} else {
						verts[vi++] = corners[0];
						verts[vi++] = corners[1];
						verts[vi++] = corners[3];
						verts[vi++] = corners[3];
						verts[vi++] = corners[1];
						verts[vi++] = corners[2];
					}
				}
				if (BNONSOLID(4)) {
					const tc2us_t* tc = &BLOCKTC(t, BLOCK_TEX_BACK, 0);
					block_vtx_t corners[4];
					corners[0].pos = POS(ix+1,   by+iy, iz), corners[0].tc = tc[0], corners[0].clr = BLOCKLIGHT( 3, 4, 6, 7,12,15,16);
					corners[1].pos = POS(  ix,   by+iy, iz), corners[1].tc = tc[1], corners[1].clr = BLOCKLIGHT( 0, 1, 3, 4,9,10,12);
					corners[2].pos = POS(  ix, by+iy+1, iz), corners[2].tc = tc[2], corners[2].clr = BLOCKLIGHT( 1, 2, 4, 5,10,11,14);
					corners[3].pos = POS(ix+1, by+iy+1, iz), corners[3].tc = tc[3], corners[3].clr = BLOCKLIGHT( 4, 5, 7, 8,14,16,17);
					if (FLIPCHECK()) {
						verts[vi++] = corners[0];
						verts[vi++] = corners[1];
						verts[vi++] = corners[2];
						verts[vi++] = corners[2];
						verts[vi++] = corners[3];
						verts[vi++] = corners[0];
					} else {
						verts[vi++] = corners[0];
						verts[vi++] = corners[1];
						verts[vi++] = corners[3];
						verts[vi++] = corners[3];
						verts[vi++] = corners[1];
						verts[vi++] = corners[2];
					}
				}

				if (blockinfo[t].flags & BLOCK_ALPHA) {
					if (vi > ALPHA_BUFFER_SIZE)
						fatal_error("Alpha buffer too small for chunk (%d, %d, %d): %zu verts, %zu blocks processed", bufx, cy, bufz, vi, nprocessed);
					*alphai = vi;
					vi = save_vi;
					verts = tesselation_buffer;
				}

				++nprocessed;

				if (vi > TESSELATION_BUFFER_SIZE)
					fatal_error("Tesselation buffer too small for chunk (%d, %d, %d): %zu verts, %zu blocks processed", bufx, cy, bufz, vi, nprocessed);
			}
		}
	}

	if (vi > 0) {
		m_create_mesh(mesh, vi, verts, ML_POS_3F | ML_TC_2US | ML_CLR_4UB, GL_STATIC_DRAW);
		m_set_material(mesh, game.materials + MAT_CHUNK);
	}
	return (vi > 0);
}

bool map_raycast(dvec3_t origin, vec3_t dir, int len, ivec3_t* hit, ivec3_t* prehit)
{
	dvec3_t blockf = { origin.x, origin.y, origin.z };
	ivec3_t block = { round(origin.x), round(origin.y), round(origin.z) };
	ivec3_t prev = {0, 0, 0};
	int step = 32;
	for (int i = 0; i < len*step; ++i) {
		block.x = round(blockf.x);
		block.y = round(blockf.y);
		block.z = round(blockf.z);
		if (block.x != prev.x || block.y != prev.y || block.z != prev.z) {
			uint8_t t = blocktype_by_coord(block);
			if (t != BLOCK_AIR) {
				//if (game.debug_mode) {
				//	ui_debug_block(prev, 0xff0000ff);
				//	ui_debug_block(block, 0xff00ff00);
				//}
				if (hit != NULL)
					*hit = block;
				if (prehit != NULL)
					*prehit = prev;
				return true;
			}
			prev = block;
		}
		blockf.x += dir.x / step;
		blockf.y += dir.y / step;
		blockf.z += dir.z / step;
	}
	return false;
}
