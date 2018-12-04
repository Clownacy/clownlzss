#include "kosinski.h"

#include <stdbool.h>
#include <stddef.h>

#include "clownlzss.h"
#include "memory_stream.h"

#define TOTAL_DESCRIPTOR_BITS 16

static MemoryStream *output_stream;
static MemoryStream *match_stream;

static unsigned short descriptor;
static unsigned int descriptor_bits_remaining;

static void FlushData(void)
{
	descriptor >>= descriptor_bits_remaining;

	MemoryStream_WriteByte(output_stream, descriptor & 0xFF);
	MemoryStream_WriteByte(output_stream, descriptor >> 8);

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
	--descriptor_bits_remaining;

	descriptor >>= 1;

	if (bit)
		descriptor |= 1 << (TOTAL_DESCRIPTOR_BITS - 1);

	if (descriptor_bits_remaining == 0)
	{
		FlushData();

		descriptor_bits_remaining = TOTAL_DESCRIPTOR_BITS;
		MemoryStream_Reset(match_stream);
	}
}

static void DoLiteral(unsigned char value, void *user)
{
	(void)user;

	PutDescriptorBit(true);
	PutMatchByte(value);
}

static void DoMatch(size_t distance, size_t length, size_t offset, void *user)
{
	(void)offset;
	(void)user;

	if (length >= 2 && length <= 5 && distance <= 256)
	{
		PutDescriptorBit(false);
		PutDescriptorBit(false);
		PutDescriptorBit((length - 2) & 2);
		PutDescriptorBit((length - 2) & 1);
		PutMatchByte(-distance);
	}
	else if (length >= 3 && length <= 9)
	{
		PutDescriptorBit(false);
		PutDescriptorBit(true);
		PutMatchByte(-distance & 0xFF);
		PutMatchByte(((-distance >> (8 - 3)) & 0xF8) | ((length - 2) & 7));
	}
	else //if (length >= 3)
	{
		PutDescriptorBit(false);
		PutDescriptorBit(true);
		PutMatchByte(-distance & 0xFF);
		PutMatchByte((-distance >> (8 - 3)) & 0xF8);
		PutMatchByte(length - 1);
	}
}

static unsigned int GetMatchCost(size_t distance, size_t length, void *user)
{
	(void)user;

	if (length >= 2 && length <= 5 && distance <= 256)
		return 8 + 2 + 2;	// Offset byte, length bits, descriptor bits
	else if (length >= 3 && length <= 9)
		return 16 + 2;		// Offset/length bytes, descriptor bits
	else if (length >= 3)
		return 24 + 2;		// Offset/length bytes, descriptor bits
	else
		return 0; 		// In the event a match cannot be compressed
}

static void FindExtraMatches(unsigned char *data, size_t data_size, size_t offset, ClownLZSS_NodeMeta *node_meta_array, void *user)
{
	(void)data;
	(void)data_size;
	(void)offset;
	(void)node_meta_array;
	(void)user;
}

static CLOWNLZSS_MAKE_FUNCTION(FindMatches, unsigned char, 0x100, 0x2000, FindExtraMatches, 8 + 1, DoLiteral, GetMatchCost, DoMatch)

unsigned char* KosinskiCompress(unsigned char *data, size_t data_size, size_t *compressed_size)
{
	output_stream = MemoryStream_Create(0x100, false);
	match_stream = MemoryStream_Create(0x10, true);
	descriptor_bits_remaining = TOTAL_DESCRIPTOR_BITS;

	FindMatches(data, data_size, NULL);

	// Terminator match
	PutDescriptorBit(false);
	PutDescriptorBit(true);
	PutMatchByte(0x00);
	PutMatchByte(0xF0);
	PutMatchByte(0x00);

	FlushData();

	unsigned char *out_buffer = MemoryStream_GetBuffer(output_stream);

	if (compressed_size)
		*compressed_size = MemoryStream_GetIndex(output_stream);

	MemoryStream_Destroy(match_stream);
	MemoryStream_Destroy(output_stream);

	return out_buffer;
}
