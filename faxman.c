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

#include "faxman.h"

#ifndef __cplusplus
#include <stdbool.h>
#endif
#include <stddef.h>

#include "clownlzss.h"
#include "common.h"
#include "memory_stream.h"

#define TOTAL_DESCRIPTOR_BITS 8

typedef struct FaxmanInstance
{
	MemoryStream *output_stream;
	MemoryStream *match_stream;

	unsigned char descriptor;
	unsigned int descriptor_bits_remaining;
	unsigned short descriptor_bits_total;
} FaxmanInstance;

static void FlushData(FaxmanInstance *instance)
{
	MemoryStream_WriteByte(instance->output_stream, instance->descriptor);

	const size_t match_buffer_size = MemoryStream_GetPosition(instance->match_stream);
	unsigned char *match_buffer = MemoryStream_GetBuffer(instance->match_stream);

	MemoryStream_WriteBytes(instance->output_stream, match_buffer, match_buffer_size);
}

static void PutMatchByte(FaxmanInstance *instance, unsigned char byte)
{
	MemoryStream_WriteByte(instance->match_stream, byte);
}

static void PutDescriptorBit(FaxmanInstance *instance, bool bit)
{
	++instance->descriptor_bits_total;

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
	FaxmanInstance *instance = (FaxmanInstance*)user;

	PutDescriptorBit(instance, 1);
	PutMatchByte(instance, value);
}

static void DoMatch(size_t distance, size_t length, size_t offset, void *user)
{
	FaxmanInstance *instance = (FaxmanInstance*)user;

	if (offset == (size_t)-1)
		distance = 0x800;

	if (length >= 2 && length <= 5 && distance <= 0x100)
	{
		PutDescriptorBit(instance, 0);
		PutDescriptorBit(instance, 0);
		PutMatchByte(instance, -distance);
		PutDescriptorBit(instance, (length - 2) & 2);
		PutDescriptorBit(instance, (length - 2) & 1);
	}
	else //if (length >= 3)
	{
		PutDescriptorBit(instance, 0);
		PutDescriptorBit(instance, 1);
		PutMatchByte(instance, (distance - 1) & 0xFF);
		PutMatchByte(instance, (((distance - 1) & 0x700) >> 3) | (length - 3));
	}
}

static unsigned int GetMatchCost(size_t distance, size_t length, void *user)
{
	(void)user;

	if (length >= 2 && length <= 5 && distance <= 0x100)
		return 2 + 8 + 2;
	else if (length >= 3)
		return 2 + 16;	// Descriptor bit, offset/length bits
	else
		return 0;
}

static void FindExtraMatches(unsigned char *data, size_t data_size, size_t offset, ClownLZSS_GraphEdge *node_meta_array, void *user)
{
	(void)user;

	if (offset < 0x800)
	{
		const size_t max_read_ahead = CLOWNLZSS_MIN(0x1F + 3, data_size - offset);

		for (size_t k = 0; k < max_read_ahead; ++k)
		{
			if (data[offset + k] == 0)
			{
				const unsigned int cost = (k + 1 >= 3) ? 2 + 16 : 0;

				if (cost && node_meta_array[offset + k + 1].u.cost > node_meta_array[offset].u.cost + cost)
				{
					node_meta_array[offset + k + 1].u.cost = node_meta_array[offset].u.cost + cost;
					node_meta_array[offset + k + 1].previous_node_index = offset;
					node_meta_array[offset + k + 1].match_length = k + 1;
					node_meta_array[offset + k + 1].match_offset = (size_t)-1;
				}
			}
			else
				break;
		}
	}
}

static CLOWNLZSS_MAKE_COMPRESSION_FUNCTION(CompressData, unsigned char, 0x1F + 3, 0x800, FindExtraMatches, 1 + 8, DoLiteral, GetMatchCost, DoMatch)

static void FaxmanCompressStream(unsigned char *data, size_t data_size, MemoryStream *output_stream, void *user)
{
	(void)user;

	const size_t file_offset = MemoryStream_GetPosition(output_stream);

	FaxmanInstance instance;
	instance.output_stream = output_stream;
	instance.match_stream = MemoryStream_Create(true);
	instance.descriptor_bits_remaining = TOTAL_DESCRIPTOR_BITS;
	instance.descriptor_bits_total = 0;

	// Blank header
	MemoryStream_WriteByte(output_stream, 0);
	MemoryStream_WriteByte(output_stream, 0);

	CompressData(data, data_size, &instance);

	instance.descriptor >>= instance.descriptor_bits_remaining;
	FlushData(&instance);

	MemoryStream_Destroy(instance.match_stream);

	unsigned char *buffer = MemoryStream_GetBuffer(output_stream);

	// Fill in header
	buffer[file_offset + 0] = instance.descriptor_bits_total & 0xFF;
	buffer[file_offset + 1] = instance.descriptor_bits_total >> 8;
}

unsigned char* ClownLZSS_FaxmanCompress(unsigned char *data, size_t data_size, size_t *compressed_size)
{
	return RegularWrapper(data, data_size, compressed_size, NULL, FaxmanCompressStream);
}

unsigned char* ClownLZSS_ModuledFaxmanCompress(unsigned char *data, size_t data_size, size_t *compressed_size, size_t module_size)
{
	return ModuledCompressionWrapper(data, data_size, compressed_size, NULL, FaxmanCompressStream, module_size, 1);
}
