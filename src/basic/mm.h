/* \brief
 *		memory allocator
 */

#ifndef __MM_H__
#define __MM_H__

#include <stdlib.h>


void  mm_set_alloctor(void* (*ptralloc)(size_t), void* (*ptrrealloc)(void*, size_t), void (*ptrfree)(void*));
void* mm_alloc(size_t size);
void* mm_realloc(void* ptr, size_t size);
void  mm_free(void* ptr);


enum EMMArea
{
	MMA_Area_0 = 0,
	MMA_Area_1,
	MMA_Area_2,
	MMA_Area_3,
	MMA_Area_4,
	MMA_Area_5,
	MMA_Area_Max,
	MMA_GLOBAL
};

void* mm_alloc_area(size_t size, enum EMMArea where);
void mm_free_area(enum EMMArea where);

#endif /* __MM_H__ */
