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

	descriptor >>= 1;

	if (bit)
		descriptor |= 1 << (TOTAL_DESCRIPTOR_BITS - 1);
}

static void DoLiteral(unsigned char value, void *user)
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

static void FindExtraMatches(unsigned char *data, size_t data_size, size_t offset, LZSSNodeMeta *node_meta_array, void *user)
{
	(void)data;
	(void)data_size;
	(void)offset;
	(void)node_meta_array;
	(void)user;
}

static MAKE_FIND_MATCHES_FUNCTION(CompressData, unsigned char, 0x40, 0x400, FindExtraMatches, 1 + 8, DoLiteral, GetMatchCost, DoMatch)

static void RocketCompressStream(unsigned char *data, size_t data_size, MemoryStream *p_output_stream)
{
	output_stream = p_output_stream;

	const size_t file_offset = MemoryStream_GetPosition(output_stream);

	match_stream = MemoryStream_Create(0x10, true);
	descriptor_bits_remaining = TOTAL_DESCRIPTOR_BITS;

	// Incomplete header
	MemoryStream_WriteByte(output_stream, data_size >> 8);
	MemoryStream_WriteByte(output_stream, data_size & 0xFF);
	MemoryStream_WriteByte(output_stream, 0);
	MemoryStream_WriteByte(output_stream, 0);

	CompressData(data, data_size, NULL);

	descriptor >>= descriptor_bits_remaining;
	FlushData();

	MemoryStream_Destroy(match_stream);

	unsigned char *buffer = MemoryStream_GetBuffer(output_stream);
	const size_t compressed_size = MemoryStream_GetPosition(output_stream) - file_offset - 2;

	// Finish header
	buffer[file_offset + 2] = compressed_size >> 8;
	buffer[file_offset + 3] = compressed_size & 0xFF;
}

unsigned char* RocketCompress(unsigned char *data, size_t data_size, size_t *compressed_size)
{
	return RegularWrapper(data, data_size, compressed_size, RocketCompressStream);
}

unsigned char* ModuledRocketCompress(unsigned char *data, size_t data_size, size_t *compressed_size, size_t module_size)
{
	return ModuledCompressionWrapper(data, data_size, compressed_size, RocketCompressStream, module_size, 1);
}
