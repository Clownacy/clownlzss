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
	MemoryStream_WriteByte(output_stream, descriptor >> 8);
	MemoryStream_WriteByte(output_stream, descriptor & 0xFF);

	const size_t match_buffer_size = MemoryStream_GetPosition(match_stream);
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
		MemoryStream_Rewind(match_stream);
	}

	--descriptor_bits_remaining;

	descriptor <<= 1;

	descriptor |= bit;
}

static void DoLiteral(unsigned short value, void *user)
{
	(void)user;

	PutDescriptorBit(0);
	PutMatchByte(value & 0xFF);
	PutMatchByte(value >> 8);
}

static void DoMatch(size_t distance, size_t length, size_t offset, void *user)
{
	(void)offset;
	(void)user;

	PutDescriptorBit(1);
	PutMatchByte(-distance);
	PutMatchByte(length - 1);
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

static CLOWNLZSS_MAKE_FIND_MATCHES_FUNCTION(CompressData, unsigned short, 0x100, 0x100, FindExtraMatches, 1 + 16, DoLiteral, GetMatchCost, DoMatch)

static void ComperCompressStream(unsigned char *data, size_t data_size, MemoryStream *p_output_stream)
{
	output_stream = p_output_stream;

	match_stream = MemoryStream_Create(0x10, true);
	descriptor_bits_remaining = TOTAL_DESCRIPTOR_BITS;

	CompressData((unsigned short*)data, data_size / sizeof(unsigned short), NULL);

	// Terminator match
	PutDescriptorBit(1);
	PutMatchByte(0);
	PutMatchByte(0);

	descriptor <<= descriptor_bits_remaining;
	FlushData();

	MemoryStream_Destroy(match_stream);
}

unsigned char* ComperCompress(unsigned char *data, size_t data_size, size_t *compressed_size)
{
	return ClownLZSS_RegularWrapper(data, data_size, compressed_size, ComperCompressStream);
}

unsigned char* ModuledComperCompress(unsigned char *data, size_t data_size, size_t *compressed_size, size_t module_size)
{
	return ClownLZSS_ModuledCompressionWrapper(data, data_size, compressed_size, ComperCompressStream, module_size, 1);
}
