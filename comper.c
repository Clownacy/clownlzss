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

#include "comper.h"

#ifndef __cplusplus
#include <stdbool.h>
#endif
#include <stddef.h>

#include "clownlzss.h"
#include "common.h"
#include "memory_stream.h"

#define TOTAL_DESCRIPTOR_BITS 16

typedef struct ComperInstance
{
	MemoryStream *output_stream;
	MemoryStream *match_stream;

	unsigned short descriptor;
	unsigned int descriptor_bits_remaining;
} ComperInstance;

static void FlushData(ComperInstance *instance)
{
	MemoryStream_WriteByte(instance->output_stream, instance->descriptor >> 8);
	MemoryStream_WriteByte(instance->output_stream, instance->descriptor & 0xFF);

	const size_t match_buffer_size = MemoryStream_GetPosition(instance->match_stream);
	unsigned char *match_buffer = MemoryStream_GetBuffer(instance->match_stream);

	MemoryStream_WriteBytes(instance->output_stream, match_buffer, match_buffer_size);
}

static void PutMatchByte(ComperInstance *instance, unsigned char byte)
{
	MemoryStream_WriteByte(instance->match_stream, byte);
}

static void PutDescriptorBit(ComperInstance *instance, bool bit)
{
	if (instance->descriptor_bits_remaining == 0)
	{
		FlushData(instance);

		instance->descriptor_bits_remaining = TOTAL_DESCRIPTOR_BITS;
		MemoryStream_Rewind(instance->match_stream);
	}

	--instance->descriptor_bits_remaining;

	instance->descriptor <<= 1;

	instance->descriptor |= bit;
}

static void DoLiteral(unsigned short value, void *user)
{
	ComperInstance *instance = (ComperInstance*)user;

	PutDescriptorBit(instance, 0);
	PutMatchByte(instance, value & 0xFF);
	PutMatchByte(instance, value >> 8);
}

static void DoMatch(size_t distance, size_t length, size_t offset, void *user)
{
	(void)offset;

	ComperInstance *instance = (ComperInstance*)user;

	PutDescriptorBit(instance, 1);
	PutMatchByte(instance, (unsigned char)-distance);
	PutMatchByte(instance, (unsigned char)(length - 1));
}

static unsigned int GetMatchCost(size_t distance, size_t length, void *user)
{
	(void)distance;
	(void)length;
	(void)user;

	return 1 + 16;	// Descriptor bit, offset/length bytes
}

static void FindExtraMatches(unsigned short *data, size_t data_size, size_t offset, ClownLZSS_GraphEdge *node_meta_array, void *user)
{
	(void)data;
	(void)data_size;
	(void)offset;
	(void)node_meta_array;
	(void)user;
}

static CLOWNLZSS_MAKE_COMPRESSION_FUNCTION(CompressData, unsigned short, 0x100, 0x100, FindExtraMatches, 1 + 16, DoLiteral, GetMatchCost, DoMatch)

static void ComperCompressStream(unsigned char *data, size_t data_size, MemoryStream *output_stream, void *user)
{
	(void)user;

	ComperInstance instance;
	instance.output_stream = output_stream;
	instance.match_stream = MemoryStream_Create(true);
	instance.descriptor_bits_remaining = TOTAL_DESCRIPTOR_BITS;

	CompressData((unsigned short*)data, data_size / sizeof(unsigned short), &instance);

	// Terminator match
	PutDescriptorBit(&instance, 1);
	PutMatchByte(&instance, 0);
	PutMatchByte(&instance, 0);

	instance.descriptor <<= instance.descriptor_bits_remaining;
	FlushData(&instance);

	MemoryStream_Destroy(instance.match_stream);
}

unsigned char* ClownLZSS_ComperCompress(unsigned char *data, size_t data_size, size_t *compressed_size)
{
	return RegularWrapper(data, data_size, compressed_size, NULL, ComperCompressStream);
}

unsigned char* ClownLZSS_ModuledComperCompress(unsigned char *data, size_t data_size, size_t *compressed_size, size_t module_size)
{
	return ModuledCompressionWrapper(data, data_size, compressed_size, NULL, ComperCompressStream, module_size, 1);
}
