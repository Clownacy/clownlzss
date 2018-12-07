#include "kosinskiplus.h"

#include <stdbool.h>
#include <stddef.h>

#include "clownlzss.h"
#include "memory_stream.h"
#include "moduled.h"

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

	descriptor <<= 1;

	descriptor |= bit;
}

static void DoLiteral(unsigned char value, void *user)
{
	(void)user;

	PutDescriptorBit(1);
	PutMatchByte(value);
}

static void DoMatch(size_t distance, size_t length, size_t offset, void *user)
{
	(void)offset;
	(void)user;

	if (length >= 2 && length <= 5 && distance <= 256)
	{
		PutDescriptorBit(0);
		PutDescriptorBit(0);
		PutMatchByte(-distance);
		PutDescriptorBit((length - 2) & 2);
		PutDescriptorBit((length - 2) & 1);
	}
	else if (length >= 3 && length <= 9)
	{
		PutDescriptorBit(0);
		PutDescriptorBit(1);
		PutMatchByte(((-distance >> (8 - 3)) & 0xF8) | ((10 - length) & 7));
		PutMatchByte(-distance & 0xFF);
	}
	else //if (length >= 10)
	{
		PutDescriptorBit(0);
		PutDescriptorBit(1);
		PutMatchByte((-distance >> (8 - 3)) & 0xF8);
		PutMatchByte(-distance & 0xFF);
		PutMatchByte(length - 9);
	}
}

static unsigned int GetMatchCost(size_t distance, size_t length, void *user)
{
	(void)user;

	if (length >= 2 && length <= 5 && distance <= 256)
		return 2 + 8 + 2;	// Descriptor bits, offset byte, length bits
	else if (length >= 3 && length <= 9)
		return 2 + 16;		// Descriptor bits, offset/length bytes
	else if (length >= 10)
		return 2 + 16 + 8;	// Descriptor bits, offset bytes, length byte
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

static CLOWNLZSS_MAKE_FUNCTION(CompressData, unsigned char, 0x100 + 8, 0x2000, FindExtraMatches, 1 + 8, DoLiteral, GetMatchCost, DoMatch)

static void KosinskiPlusCompressStream(unsigned char *data, size_t data_size, MemoryStream *p_output_stream)
{
	output_stream = p_output_stream;

	match_stream = MemoryStream_Create(0x10, true);
	descriptor_bits_remaining = TOTAL_DESCRIPTOR_BITS;

	CompressData(data, data_size, NULL);

	// Terminator match
	PutDescriptorBit(0);
	PutDescriptorBit(1);
	PutMatchByte(0xF0);
	PutMatchByte(0x00);
	PutMatchByte(0x00);

	descriptor <<= descriptor_bits_remaining;
	FlushData();

	MemoryStream_Destroy(match_stream);
}

unsigned char* KosinskiPlusCompress(unsigned char *data, size_t data_size, size_t *compressed_size)
{
	MemoryStream *output_stream = MemoryStream_Create(0x1000, false);

	KosinskiPlusCompressStream(data, data_size, output_stream);

	unsigned char *out_buffer = MemoryStream_GetBuffer(output_stream);

	if (compressed_size)
		*compressed_size = MemoryStream_GetIndex(output_stream);

	MemoryStream_Destroy(output_stream);

	return out_buffer;
}

unsigned char* ModuledKosinskiPlusCompress(unsigned char *data, size_t data_size, size_t *compressed_size, size_t module_size)
{
	return ModuledCompress(data, data_size, compressed_size, KosinskiPlusCompressStream, module_size, 1);
}
