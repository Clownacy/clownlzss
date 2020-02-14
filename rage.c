/*
	(C) 2020 Clownacy

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

// This is the worst thing I've ever written - it uses clownlzss to compress 
// files in Streets of Rage 2's 'Rage' format... which is RLE.

#include "rage.h"

#ifndef __cplusplus
#include <stdbool.h>
#endif
#include <stddef.h>

#include "clownlzss.h"
#include "common.h"
#include "memory_stream.h"

#define TOTAL_DESCRIPTOR_BITS 8

typedef struct RageInstance
{
	MemoryStream *output_stream;
	unsigned char *data;
} RageInstance;

static void PutMatchByte(RageInstance *instance, unsigned char byte)
{
	MemoryStream_WriteByte(instance->output_stream, byte);
}

static void DoLiteral(unsigned char value, void *user)
{
	(void)value;
	(void)user;

	// This should never be ran
}

static void DoMatch(size_t distance, size_t length, size_t offset, void *user)
{
	(void)distance;

	RageInstance *instance = (RageInstance*)user;

	if (distance == 0)
	{
		// Uncompressed run
		if (length > 0x1F)
		{
			PutMatchByte(instance, 0x20 | ((length >> 8) & 0x1F));
			PutMatchByte(instance, length & 0xFF);
		}
		else
		{
			PutMatchByte(instance, length);
		}

		for (size_t i = 0; i < length; ++i)
			PutMatchByte(instance, instance->data[offset + i]);
	}
	else if ((offset & 0xFFFFFF00) == 0xFFFFFF00)
	{
		// RLE-match
		length -= 4;

		if (length > 0xF)
		{
			PutMatchByte(instance, 0x40 | 0x10 | ((length >> 8) & 0xF));
			PutMatchByte(instance, length & 0xFF);
		}
		else
		{
			PutMatchByte(instance, 0x40 | (length & 0xF));
		}

		PutMatchByte(instance, offset & 0xFF);
	}
	else
	{
		// Dictionary-match
		length -= 4;

		// The first match can only encode 7 bytes
		size_t thing = length > 3 ? 3 : length;

		PutMatchByte(instance, 0x80 | (thing << 5) | ((distance >> 8) & 0x1F));
		PutMatchByte(instance, distance & 0xFF);

		length -= thing;

		// If there are still more bytes in this match, do them in blocks of 0x1F bytes
		while (length != 0)
		{
			thing = length > 0x1F ? 0x1F : length;

			PutMatchByte(instance, 0x60 | thing);
			length -= thing;
		}
	}
}

static unsigned int GetMatchCost(size_t distance, size_t length, void *user)
{
	(void)distance;
	(void)length;
	(void)user;

	if (length >= 4)
		return (2 + (((length - 4 - 3) + (0x1F - 1)) / 0x1F)) * 8;
	else
		return 0;
}

static void FindExtraMatches(unsigned char *data, size_t data_size, size_t offset, ClownLZSS_GraphEdge *node_meta_array, void *user)
{
	size_t max_read_ahead;

	(void)user;

	// Look for RLE-matches
	max_read_ahead = CLOWNLZSS_MIN(0xFFF + 4, data_size - offset);

	for (size_t k = 0; k < max_read_ahead; ++k)
	{
		if (data[offset + k] == data[offset])
		{
			unsigned int cost;

			if (k + 1 < 4)
				cost = 0;
			else
				cost = ((k + 1 - 4 > 0xF ? 2 : 1) + 1) * 8;

			if (cost && node_meta_array[offset + k + 1].u.cost > node_meta_array[offset].u.cost + cost)
			{
				node_meta_array[offset + k + 1].u.cost = node_meta_array[offset].u.cost + cost;
				node_meta_array[offset + k + 1].previous_node_index = offset;
				node_meta_array[offset + k + 1].match_length = k + 1;
				node_meta_array[offset + k + 1].match_offset = 0xFFFFFF00 | data[offset];	// Horrible hack, like the rest of this compressor
			}
		}
		else
			break;
	}

	// Add uncompressed runs
	max_read_ahead = CLOWNLZSS_MIN(0x1FFF, data_size - offset);

	for (size_t k = 0; k < max_read_ahead; ++k)
	{
		const unsigned int cost = (k + 1 + (k + 1 > 0x1F ? 2 : 1)) * 8;

		if (cost && node_meta_array[offset + k + 1].u.cost > node_meta_array[offset].u.cost + cost)
		{
			node_meta_array[offset + k + 1].u.cost = node_meta_array[offset].u.cost + cost;
			node_meta_array[offset + k + 1].previous_node_index = offset;
			node_meta_array[offset + k + 1].match_length = k + 1;
			node_meta_array[offset + k + 1].match_offset = offset;	// Points at itself
		}
	}
}

static CLOWNLZSS_MAKE_COMPRESSION_FUNCTION(CompressData, unsigned char, 0xFFFFFFFF/*dictionary-matches can be infinite*/, 0x1FFF, FindExtraMatches, 0xFFFFFFF/*dummy*/, DoLiteral, GetMatchCost, DoMatch)

static void RageCompressStream(unsigned char *data, size_t data_size, MemoryStream *output_stream, void *user)
{
	(void)user;

	RageInstance instance;
	instance.output_stream = output_stream;
	instance.data = data;

	const size_t file_offset = MemoryStream_GetPosition(output_stream);

	// Blank header
	MemoryStream_WriteByte(output_stream, 0);
	MemoryStream_WriteByte(output_stream, 0);

	CompressData(data, data_size, &instance);

	unsigned char *buffer = MemoryStream_GetBuffer(output_stream);
	const size_t compressed_size = MemoryStream_GetPosition(output_stream) - file_offset;

	// Fill in header
	buffer[file_offset + 0] = compressed_size & 0xFF;
	buffer[file_offset + 1] = (compressed_size >> 8) & 0xFF;
}

unsigned char* ClownLZSS_RageCompress(unsigned char *data, size_t data_size, size_t *compressed_size)
{
	return RegularWrapper(data, data_size, compressed_size, NULL, RageCompressStream);
}

unsigned char* ClownLZSS_ModuledRageCompress(unsigned char *data, size_t data_size, size_t *compressed_size, size_t module_size)
{
	return ModuledCompressionWrapper(data, data_size, compressed_size, NULL, RageCompressStream, module_size, 1);
}
