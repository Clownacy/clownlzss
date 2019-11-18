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

#include "saxman.h"

#ifndef __cplusplus
#include <stdbool.h>
#endif
#include <stddef.h>

#include "clownlzss.h"
#include "common.h"
#include "memory_stream.h"

#define TOTAL_DESCRIPTOR_BITS 8

typedef struct SaxmanInstance
{
	MemoryStream *output_stream;
	MemoryStream *match_stream;

	unsigned char descriptor;
	unsigned int descriptor_bits_remaining;
} SaxmanInstance;

static void FlushData(SaxmanInstance *instance)
{
	MemoryStream_WriteByte(instance->output_stream, instance->descriptor);

	const size_t match_buffer_size = MemoryStream_GetPosition(instance->match_stream);
	unsigned char *match_buffer = MemoryStream_GetBuffer(instance->match_stream);

	MemoryStream_WriteBytes(instance->output_stream, match_buffer, match_buffer_size);
}

static void PutMatchByte(SaxmanInstance *instance, unsigned char byte)
{
	MemoryStream_WriteByte(instance->match_stream, byte);
}

static void PutDescriptorBit(SaxmanInstance *instance, bool bit)
{
	if (instance->descriptor_bits_remaining == 0)
	{
		FlushData(instance);

		instance->descriptor_bits_remaining = TOTAL_DESCRIPTOR_BITS;
		MemoryStream_Rewind(instance->match_stream);
	}

	--instance->descriptor_bits_remaining;

	instance->descriptor >>= 1;

	if (bit)
		instance->descriptor |= 1 << (TOTAL_DESCRIPTOR_BITS - 1);
}

static void DoLiteral(unsigned char value, void *user)
{
	SaxmanInstance *instance = (SaxmanInstance*)user;

	PutDescriptorBit(instance, 1);
	PutMatchByte(instance, value);
}

static void DoMatch(size_t distance, size_t length, size_t offset, void *user)
{
	(void)distance;

	SaxmanInstance *instance = (SaxmanInstance*)user;

	PutDescriptorBit(instance, 0);
	PutMatchByte(instance, (offset - 0x12) & 0xFF);
	PutMatchByte(instance, (unsigned char)((((offset - 0x12) & 0xF00) >> 4) | (length - 3)));
}

static unsigned int GetMatchCost(size_t distance, size_t length, void *user)
{
	(void)distance;
	(void)length;
	(void)user;

	if (length >= 3)
		return 1 + 16;	// Descriptor bit, offset/length bits
	else
		return 0;
}

static void FindExtraMatches(unsigned char *data, size_t data_size, size_t offset, ClownLZSS_GraphEdge *node_meta_array, void *user)
{
	(void)user;

	if (offset < 0x1000)
	{
		const size_t max_read_ahead = CLOWNLZSS_MIN(0x12, data_size - offset);

		for (size_t k = 0; k < max_read_ahead; ++k)
		{
			if (data[offset + k] == 0)
			{
				const unsigned int cost = GetMatchCost(0, k + 1, user);

				if (cost && node_meta_array[offset + k + 1].u.cost > node_meta_array[offset].u.cost + cost)
				{
					node_meta_array[offset + k + 1].u.cost = node_meta_array[offset].u.cost + cost;
					node_meta_array[offset + k + 1].previous_node_index = offset;
					node_meta_array[offset + k + 1].match_length = k + 1;
					node_meta_array[offset + k + 1].match_offset = 0xFFF;
				}
			}
			else
				break;
		}
	}
}

static CLOWNLZSS_MAKE_COMPRESSION_FUNCTION(CompressData, unsigned char, 0x12, 0x1000, FindExtraMatches, 1 + 8, DoLiteral, GetMatchCost, DoMatch)

static void SaxmanCompressStream(unsigned char *data, size_t data_size, MemoryStream *output_stream, void *user)
{
	const bool header = *(bool*)user;

	SaxmanInstance instance;
	instance.output_stream = output_stream;
	instance.match_stream = MemoryStream_Create(true);
	instance.descriptor_bits_remaining = TOTAL_DESCRIPTOR_BITS;

	const size_t file_offset = MemoryStream_GetPosition(output_stream);

	if (header)
	{
		// Blank header
		MemoryStream_WriteByte(output_stream, 0);
		MemoryStream_WriteByte(output_stream, 0);
	}

	CompressData(data, data_size, &instance);

	instance.descriptor >>= instance.descriptor_bits_remaining;
	FlushData(&instance);

	MemoryStream_Destroy(instance.match_stream);

	if (header)
	{
		unsigned char *buffer = MemoryStream_GetBuffer(output_stream);
		const size_t compressed_size = MemoryStream_GetPosition(output_stream) - file_offset - 2;

		// Fill in header
		buffer[file_offset + 0] = compressed_size & 0xFF;
		buffer[file_offset + 1] = (compressed_size >> 8) & 0xFF;
	}
}

unsigned char* ClownLZSS_SaxmanCompress(unsigned char *data, size_t data_size, size_t *compressed_size, bool header)
{
	return RegularWrapper(data, data_size, compressed_size, &header, SaxmanCompressStream);
}

unsigned char* ClownLZSS_ModuledSaxmanCompress(unsigned char *data, size_t data_size, size_t *compressed_size, bool header, size_t module_size)
{
	return ModuledCompressionWrapper(data, data_size, compressed_size, &header, SaxmanCompressStream, module_size, 1);
}
