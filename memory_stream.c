/*
	(C) 2018-2019 Clownacy

	This software is provided 'as-is', without any express or implied
	warranty.  In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not
	   claim that you wrote the original software. If you use this software
	   in a product, an acknowledgment in the product documentation would be
	   appreciated but is not required.
	2. Altered source versions must be plainly marked as such, and must not be
	   misrepresented as being the original software.
	3. This notice may not be removed or altered from any source distribution.
*/

#include "memory_stream.h"

#ifndef __cplusplus
#include <stdbool.h>
#endif
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

struct MemoryStream
{
	unsigned char *buffer;
	size_t position;
	size_t end;
	size_t size;
	bool free_buffer_when_destroyed;
};

static void ResizeIfNeeded(MemoryStream *memory_stream, size_t minimum_needed_size)
{
	if (minimum_needed_size > memory_stream->size)
	{
		size_t new_size = 1;
		while (new_size < minimum_needed_size)
			new_size <<= 1;

		memory_stream->buffer = (unsigned char*)realloc(memory_stream->buffer, new_size);
		memset(memory_stream->buffer + memory_stream->size, 0, new_size - memory_stream->size);
		memory_stream->size = new_size;
	}

	if (minimum_needed_size > memory_stream->end)
		memory_stream->end = minimum_needed_size;
}

MemoryStream* MemoryStream_Create(bool free_buffer_when_destroyed)
{
	MemoryStream *memory_stream = (MemoryStream*)malloc(sizeof(MemoryStream));
	memory_stream->buffer = NULL;
	memory_stream->position = 0;
	memory_stream->end = 0;
	memory_stream->size = 0;
	memory_stream->free_buffer_when_destroyed = free_buffer_when_destroyed;
	return memory_stream;
}

void MemoryStream_Destroy(MemoryStream *memory_stream)
{
	if (memory_stream->free_buffer_when_destroyed)
		free(memory_stream->buffer);

	free(memory_stream);
}

void MemoryStream_WriteByte(MemoryStream *memory_stream, unsigned char byte)
{
	ResizeIfNeeded(memory_stream, memory_stream->position + 1);

	memory_stream->buffer[memory_stream->position++] = byte;
}

void MemoryStream_WriteBytes(MemoryStream *memory_stream, unsigned char *bytes, size_t length)
{
	ResizeIfNeeded(memory_stream, memory_stream->position + length);

	memcpy(&memory_stream->buffer[memory_stream->position], bytes, length);
	memory_stream->position += length;
}

unsigned char* MemoryStream_GetBuffer(MemoryStream *memory_stream)
{
	return memory_stream->buffer;
}

size_t MemoryStream_GetPosition(MemoryStream *memory_stream)
{
	return memory_stream->position;
}

void MemoryStream_SetPosition(MemoryStream *memory_stream, ptrdiff_t offset, enum MemoryStream_Origin origin)
{
	switch (origin)
	{
		case MEMORYSTREAM_START:
			memory_stream->position = (size_t)offset;
			break;
		case MEMORYSTREAM_CURRENT:
			memory_stream->position = (size_t)(memory_stream->position + offset);
			break;
		case MEMORYSTREAM_END:
			memory_stream->position = (size_t)(memory_stream->end + offset);
			break;
	}
}

void MemoryStream_Rewind(MemoryStream *memory_stream)
{
	memory_stream->position = 0;
}
