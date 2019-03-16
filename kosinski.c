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
	MemoryStream_WriteByte(output_stream, descriptor & 0xFF);
	MemoryStream_WriteByte(output_stream, descriptor >> 8);

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
	--descriptor_bits_remaining;

	descriptor >>= 1;

	if (bit)
		descriptor |= 1 << (TOTAL_DESCRIPTOR_BITS - 1);

	if (descriptor_bits_remaining == 0)
	{
		FlushData();

		descriptor_bits_remaining = TOTAL_DESCRIPTOR_BITS;
		MemoryStream_Rewind(match_stream);
	}
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
		PutDescriptorBit((length - 2) & 2);
		PutDescriptorBit((length - 2) & 1);
		PutMatchByte(-distance);
	}
	else if (length >= 3 && length <= 9)
	{
		PutDescriptorBit(0);
		PutDescriptorBit(1);
		PutMatchByte(-distance & 0xFF);
		PutMatchByte(((-distance >> (8 - 3)) & 0xF8) | ((length - 2) & 7));
	}
	else //if (length >= 3)
	{
		PutDescriptorBit(0);
		PutDescriptorBit(1);
		PutMatchByte(-distance & 0xFF);
		PutMatchByte((-distance >> (8 - 3)) & 0xF8);
		PutMatchByte(length - 1);
	}
}

static unsigned int GetMatchCost(size_t distance, size_t length, void *user)
{
	(void)user;

	if (length >= 2 && length <= 5 && distance <= 256)
		return 2 + 2 + 8;	// Descriptor bits, length bits, offset byte
	else if (length >= 3 && length <= 9)
		return 2 + 16;		// Descriptor bits, offset/length bytes
	else if (length >= 3)
		return 2 + 16 + 8;	// Descriptor bits, offset bytes, length byte
	else
		return 0; 		// In the event a match cannot be compressed
}

static void FindExtraMatches(unsigned char *data, size_t data_size, size_t offset, ClownLZSS_GraphEdge *node_meta_array, void *user)
{
	(void)data;
	(void)data_size;
	(void)offset;
	(void)node_meta_array;
	(void)user;
}

static CLOWNLZSS_MAKE_FIND_MATCHES_FUNCTION(CompressData, unsigned char, 0x100, 0x2000, FindExtraMatches, 1 + 8, DoLiteral, GetMatchCost, DoMatch)

static void KosinskiCompressStream(unsigned char *data, size_t data_size, MemoryStream *p_output_stream)
{
	output_stream = p_output_stream;

	match_stream = MemoryStream_Create(0x10, true);
	descriptor_bits_remaining = TOTAL_DESCRIPTOR_BITS;

	CompressData(data, data_size, NULL);

	// Terminator match
	PutDescriptorBit(0);
	PutDescriptorBit(1);
	PutMatchByte(0x00);
	PutMatchByte(0xF0);
	PutMatchByte(0x00);

	descriptor >>= descriptor_bits_remaining;
	FlushData();

	MemoryStream_Destroy(match_stream);
}

unsigned char* KosinskiCompress(unsigned char *data, size_t data_size, size_t *compressed_size)
{
	return ClownLZSS_RegularWrapper(data, data_size, compressed_size, KosinskiCompressStream);
}

unsigned char* ModuledKosinskiCompress(unsigned char *data, size_t data_size, size_t *compressed_size, size_t module_size)
{
	return ClownLZSS_ModuledCompressionWrapper(data, data_size, compressed_size, KosinskiCompressStream, module_size, 0x10);
}
