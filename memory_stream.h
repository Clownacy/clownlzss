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

#pragma once 

#ifndef __cplusplus
#include <stdbool.h>
#endif
#include <stddef.h>

typedef struct MemoryStream MemoryStream;

enum MemoryStream_Origin
{
	MEMORYSTREAM_START,
	MEMORYSTREAM_CURRENT,
	MEMORYSTREAM_END
};

MemoryStream* MemoryStream_Create(bool free_buffer_when_destroyed);
void MemoryStream_Destroy(MemoryStream *memory_stream);
void MemoryStream_WriteByte(MemoryStream *memory_stream, unsigned char byte);
void MemoryStream_WriteBytes(MemoryStream *memory_stream, unsigned char *bytes, size_t byte_count);
unsigned char* MemoryStream_GetBuffer(MemoryStream *memory_stream);
size_t MemoryStream_GetPosition(MemoryStream *memory_stream);
void MemoryStream_SetPosition(MemoryStream *memory_stream, ptrdiff_t offset, enum MemoryStream_Origin origin);
void MemoryStream_Rewind(MemoryStream *memory_stream);
