#pragma once

/* create all threads */
void threadsInit(void);

/* stop all threads */
void threadsExit(void);

/* process results from completed jobs */
void threadsUpdate(void);

/* add a job to generate or load chunk (x/bufx, z/bufz) */
void threadsGenerateChunk(int x, int z, int bufx, int bufz);

/* add a job to tesselate chunk (x/bufx, z/bufz) */
void threadsTesselateChunk(int x, int z, int bufx, int bufz);

