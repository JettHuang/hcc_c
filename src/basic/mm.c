/* \brief
	memory allocator implementation.
*/

#include "mm.h"
#include <assert.h>


static void* (*g_alloc)(size_t) = malloc;
static void* (*g_realloc)(void*, size_t) = realloc;
static void (*g_free)(void*) = free;

void mm_set_alloctor(void* (*ptralloc)(size_t), void* (*ptrrealloc)(void*, size_t), void (*ptrfree)(void*))
{
	g_alloc = ptralloc;
	g_realloc = ptrrealloc;
	g_free = ptrfree;
}

void* mm_alloc(size_t size)
{
	if (g_alloc)
	{
		return g_alloc(size);
	}

	return NULL;
}

void* mm_realloc(void* ptr, size_t size)
{
	if (g_realloc)
	{
		return g_realloc(ptr, size);
	}

	return NULL;
}

void  mm_free(void* ptr)
{
	if (g_free)
	{
		g_free(ptr);
	}
}


#define MM_NORMAL_CHUNK_SIZE		(1024*8)
#define MM_SINGLE_CHUNK_SIZE		(1024*2)
#define MM_ALLOC_ALIGN				(4)

#define ALIGN_PTR(ptr, a)		((char*)(((char*)(ptr) - (char*)0 + (a - 1)) & (~(a - 1))))

typedef struct tagMmChunk
{
	struct tagMmChunk* _next;
	size_t	_capbytes;
	size_t	_freebytes;
	char*	_ptrfree;
} FMmChunk;

typedef struct tagMmArea
{
	FMmChunk* _freechunk;
} FMmArea;

static FMmChunk* new_chunk(size_t capbytes)
{
	FMmChunk* chunk;

	chunk = mm_alloc(sizeof(FMmChunk) + capbytes);
	if (!chunk) return NULL;

	chunk->_next = NULL;
	chunk->_capbytes = capbytes;
	chunk->_freebytes = capbytes;
	chunk->_ptrfree = ((char*)chunk) + sizeof(FMmChunk);
	return chunk;
}

static void* alloc_area(FMmArea* area, size_t size)
{
	FMmChunk* chunk, **where;
	char* ptralign;
	size_t waste;

	assert(area);
	if (size >= MM_SINGLE_CHUNK_SIZE)
	{
		if (!(chunk = new_chunk(size + MM_ALLOC_ALIGN)))
		{
			return NULL;
		}

		ptralign = (char*)ALIGN_PTR(chunk->_ptrfree, MM_ALLOC_ALIGN);
		waste = ptralign - chunk->_ptrfree;
		assert((size + waste) <= chunk->_freebytes);
		chunk->_ptrfree = ptralign + size;
		chunk->_freebytes -= (size + waste);

		where = area->_freechunk ? &(area->_freechunk->_next) : &(area->_freechunk);
		chunk->_next = *where;
		*where = chunk;
		
		return ptralign;
	}
	
	chunk = area->_freechunk;
	if (chunk)
	{
		ptralign = (char*)ALIGN_PTR(chunk->_ptrfree, MM_ALLOC_ALIGN);
		waste = ptralign - chunk->_ptrfree;
		if ((size + waste) > chunk->_freebytes) // no enough space ?
		{
			chunk = NULL;
		}
	}
	if (!chunk)
	{
		chunk = new_chunk(MM_NORMAL_CHUNK_SIZE + MM_ALLOC_ALIGN);
	}
	if (!chunk)
	{
		return NULL;
	}

	ptralign = (char*)ALIGN_PTR(chunk->_ptrfree, MM_ALLOC_ALIGN);
	waste = ptralign - chunk->_ptrfree;
	assert((size + waste) <= chunk->_freebytes);
	chunk->_ptrfree = ptralign + size;
	chunk->_freebytes -= (size + waste);

	if (chunk != area->_freechunk)
	{
		chunk->_next = area->_freechunk;
		area->_freechunk = chunk;
	}

	return ptralign;
}

static void free_area(FMmArea* area)
{
	FMmChunk *chunk, *tmp;

	assert(area);
	chunk = area->_freechunk;
	while (chunk)
	{
		tmp = chunk;
		chunk = chunk->_next;
		mm_free(tmp);
	} /* end while */

	area->_freechunk = NULL;
}

static int get_chunks_count_area(FMmArea* area)
{
	FMmChunk* chunk;
	int count = 0;

	assert(area);
	chunk = area->_freechunk;
	while (chunk)
	{
		count++;
		chunk = chunk->_next;
	} /* end while */

	return count;
}

static FMmArea g_areas[MMA_Area_Max] = { { NULL } };

void* mm_alloc_area(size_t size, enum EMMArea where)
{
	if (size == 0) {
		return NULL;
	}

	assert(where >= MMA_Area_0 && where < MMA_Area_Max);
	return alloc_area(&g_areas[(int)where], size);
}

void mm_free_area(enum EMMArea where)
{
	assert(where >= MMA_Area_0 && where < MMA_Area_Max);

	free_area(&g_areas[(int)where]);
}

int mm_get_area_chunks(enum EMMArea where)
{
	assert(where >= MMA_Area_0 && where < MMA_Area_Max);

	return get_chunks_count_area(&g_areas[(int)where]);
}
