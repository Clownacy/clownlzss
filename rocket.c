#include "rocket.h"

#include <stdbool.h>
#include <stddef.h>

#include "clownlzss.h"
#include "memory_stream.h"

#define TOTAL_DESCRIPTOR_BITS 8

static MemoryStream *output_stream;
static MemoryStream *match_stream;

static unsigned char descriptor;
static unsigned int descriptor_bits_remaining;

static void FlushData(void)
{
	MemoryStream_WriteByte(output_stream, descriptor);

	const size_t match_buffer_size = MemoryStream_GetIndex(match_stream);
	unsigned char *match_buffer = MemoryStream_GetBuffer(match_stream);

	MemoryStream_WriteBytes(output_stream, match_buffer, match_buffer_size);
}

static void PutMatchByte(unsigned char byte)
{
	MemoryStream_WriteByte(match_stream, byte);
}

static void PutDescriptorBit(bool bit)
{
	if (descriptor_bits_remaining == 0)
	{
		FlushData();

		descriptor_bits_remaining = TOTAL_DESCRIPTOR_BITS;
		MemoryStream_Reset(match_stream);
	}

	--descriptor_bits_remaining;

	descriptor >>= 1;

	if (bit)
		descriptor |= 1 << (TOTAL_DESCRIPTOR_BITS - 1);
}

static void DoLiteral(unsigned short value, void *user)
{
	(void)user;

	PutDescriptorBit(1);
	PutMatchByte(value);
}

static void DoMatch(size_t distance, size_t length, size_t offset, void *user)
{
	(void)distance;
	(void)user;

	const unsigned short offset_adjusted = (offset + 0x3C0) & 0x3FF;

	PutDescriptorBit(0);
	PutMatchByte(((offset_adjusted >> 8) & 3) | ((length - 1) << 2));
	PutMatchByte(offset_adjusted & 0xFF);
}

static unsigned int GetMatchCost(size_t distance, size_t length, void *user)
{
	(void)distance;
	(void)length;
	(void)user;

	return 1 + 16;	// Descriptor bit, offset/length bytes
}

static void FindExtraMatches(unsigned char *data, size_t data_size, size_t offset, ClownLZSS_NodeMeta *node_meta_array, void *user)
{
	(void)data;
	(void)data_size;
	(void)offset;
	(void)node_meta_array;
	(void)user;
}

static CLOWNLZSS_MAKE_FUNCTION(FindMatches, unsigned char, 0x40, 0x400, FindExtraMatches, 1 + 8, DoLiteral, GetMatchCost, DoMatch)

unsigned char* RocketCompress(unsigned char *data, size_t data_size, size_t *out_compressed_size)
{
	output_stream = MemoryStream_Create(0x100, false);
	match_stream = MemoryStream_Create(0x10, true);
	descriptor_bits_remaining = TOTAL_DESCRIPTOR_BITS;

	// Blank header
	MemoryStream_WriteByte(output_stream, 0);
	MemoryStream_WriteByte(output_stream, 0);
	MemoryStream_WriteByte(output_stream, 0);
	MemoryStream_WriteByte(output_stream, 0);

	FindMatches(data, data_size, NULL);

	descriptor >>= descriptor_bits_remaining;
	FlushData();

	unsigned char *out_buffer = MemoryStream_GetBuffer(output_stream);

	const size_t compressed_size = MemoryStream_GetIndex(output_stream);

	if (out_compressed_size)
		*out_compressed_size = compressed_size;

	MemoryStream_Destroy(match_stream);
	MemoryStream_Destroy(output_stream);

	// Fill in header
	out_buffer[0] = data_size >> 8;
	out_buffer[1] = data_size & 0xFF;
	out_buffer[2] = compressed_size >> 8;
	out_buffer[3] = compressed_size & 0xFF;

	return out_buffer;
}
