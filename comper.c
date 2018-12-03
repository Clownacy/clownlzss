#include "comper.h"

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
	descriptor <<= descriptor_bits_remaining;

	MemoryStream_WriteByte(output_stream, descriptor >> 8);
	MemoryStream_WriteByte(output_stream, descriptor & 0xFF);

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

	descriptor <<= 1;

	descriptor |= bit;
}

static void DoLiteral(unsigned char value, void *user)
{
	(void)user;

	PutDescriptorBit(false);
	PutMatchByte(value & 0xFF);
	PutMatchByte(value >> 8);
}

static void DoMatch(size_t distance, size_t length, void *user)
{
	(void)user;

	PutDescriptorBit(true);
	PutMatchByte(-distance);
	PutMatchByte(length - 1);
}

static unsigned int GetMatchCost(size_t distance, size_t length, void *user)
{
	(void)distance;
	(void)length;
	(void)user;

	return 16 + 1;
}

static CLOWNLZSS_MAKE_FUNCTION(FindMatchesComper, unsigned short, 0x100, 0x100, 16 + 1, DoLiteral, GetMatchCost, DoMatch)

unsigned char* ComperCompress(unsigned char *data, size_t data_size, size_t *compressed_size)
{
	output_stream = MemoryStream_Init(0x100);
	match_stream = MemoryStream_Init(0x10);
	descriptor_bits_remaining = TOTAL_DESCRIPTOR_BITS;

	FindMatchesComper((unsigned short*)data, data_size, NULL);

	// Terminator match
	PutDescriptorBit(true);
	PutMatchByte(0x00);
	PutMatchByte(0x00);

	FlushData();

	unsigned char *out_buffer = MemoryStream_GetBuffer(output_stream);

	if (compressed_size)
		*compressed_size = MemoryStream_GetIndex(output_stream);

	free(match_stream);
	free(output_stream);

	return out_buffer;
}
