# R O A M

A first-person exploration game with voxel terrain a la Minecraft. At
least, that's the eventual goal. Right now it's a non-functional
Minecraft-like, but I'm going to go a different direction, I think.

Released under the most liberal license I could find (see LICENSE).

![screenshot](http://33.media.tumblr.com/3da5329f8ec1803e36ba3714187e169f/tumblr_ndqpxtSMMc1qbhw3go1_1280.jpg)

## Features

Right now, not much.

* Day/night cycle
* Place/remove blocks
* Infinite terrain

## Dependencies

* A C11-capable compiler (GCC or clang)
* OpenGL 3.3
* SDL 2
* GLEW
* CMake

## map / chunk structure redesign

So right now I only have one big cube of block data. However, that's
very limited - I'd like to extend the limits on world height / depth
by quite a bit, but every block extension in the Y direction costs XZ
blocks of storage, so the cost is needlessly high (because this is
something that wouldn't actually be used most of the time).
Also, sub-chunks that are all air shouldn't need to be stored at all.

Instead, I want to change this:

1. keep a virtual address table of chunks, not blocks.
2. Each chunk has an array of pointers to subchunks of blocks.

To look up a specific block, we calculate the virtual address of the
chunk that contain that block, and then look up the block within the
chunk. In there, we need to figure out if the sub-chunk is allocated
at all, and if so get the block from there. So block lookup becomes
more expensive, but the benefit is that we can dig extremely deep and
have more terrain variation (valleys, mountains, etc).

Downside is that when meshing, block lookup will be a lot more
awkward. we basically need to locate the block data for all the
surrounding chunks as well. That's mostly just a performance problem,
and not likely to be a big performance problem.

Block data is always managed in 16x16x16 chunks, so a simple
single-sized allocator with freelist in the blocks themselves would
work. But to keep the chunks statically sized we need to decide on a
maximum size anyway (although less limited). Well, doable.

I could chain-allocate larger blocks.

TODO:

* Proper memory allocator / memory tracking
* Better visualisation
* builtin HTTP/REST server for tweak controls
* remove use of lua?
* entities
* better terrain generation
  * biomes
  * features - plants, structures
  * terrain editor
* some kind of ui handling (imgui?)
* entities - friendly animals, hostile animals
* inventory, tools, weapons, health
* audio
* physics
* fix controls



## thoughts on terrain generation

to generate chunk (X, Z) to level N+1,
require that chunks (X+-1,Z+-1) are generated to level N

Level 0: Biome determination for each block in chunk (smoothed 2d function (temperature+humidity+elevation maps?)
Level 1: Height map generation - blending between biomes is the tricky
part here... want to allow completely different functions for
different biomes, but need to cross-blend somehow.
Level 2: Ore placement
Level 3: Caves generation - not sure how/if I want to do this..
Level 4: Water generation - lakes, rivers - use humidity map
Level 5: Trees generation - again, humidity, biome..
Level 6: Structure generation
Level 7: Decoration (plants, mushrooms)

