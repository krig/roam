#include "common.h"
#include "threads.h"
#include "math3d.h"
#include "game.h"
#include "voxelmap.h"

#define NUM_WORKERS 4

enum thread_job {
	JOB_LOAD,
	JOB_GENERATE,
	JOB_TESSELATE
};

struct threads_job {
	int type;
	int x;
	int z;
	int bufx;
	int bufz;
};

/* create all threads

   each in-memory chunk region has its own work queue
   each worker picks a job from one of the queues or sleeps
 */
void threadsInit() {
}

/* stop all threads */
void threadsExit() {
}

/* process results from finished jobs */
void threadsUpdate() {
}

/* add a job to generate or load chunk (x/bufx, z/bufz)
   generation doesn't need to read outside chunk
 */
void threadsGenerateChunk(int x, int z, int bufx, int bufz) {
}

/* add a job to tesselate chunk (x/bufx, z/bufz)
   tesselation needs to read outside chunk, but it
   could synchronously copy data from surrounding chunks
   before beginning and use that instead. Alternatively
   modification of border blocks in surrounding chunks will
   trigger a later re-tesselation anyway, so... yeah. Doesn't
   really matter. plus the useless tesselation can be discarded
   before uploaded to the GPU.
 */
void threadsTesselateChunk(int x, int z, int bufx, int bufz) {
}
