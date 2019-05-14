#include "chameleon.h"

#include <stdbool.h>
#include <stddef.h>

#include "clownlzss.h"
#include "common.h"
#include "memory_stream.h"

#define TOTAL_DESCRIPTOR_BITS 8

static MemoryStream *match_stream;
static MemoryStream *descriptor_stream;

static unsigned char descriptor;
static unsigned int descriptor_bits_remaining;

static void PutMatchByte(unsigned char byte)
{
	MemoryStream_WriteByte(match_stream, byte);
}

static void PutDescriptorBit(bool bit)
{
	if (descriptor_bits_remaining == 0)
	{
		MemoryStream_WriteByte(descriptor_stream, descriptor);

		descriptor_bits_remaining = TOTAL_DESCRIPTOR_BITS;
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

	if (length >= 2 && length <= 3 && distance < 256)
	{
		PutDescriptorBit(0);
		PutDescriptorBit(0);
		PutMatchByte(distance);
		PutDescriptorBit(length == 3);
	}
	else if (length >= 3 && length <= 5)
	{
		PutDescriptorBit(0);
		PutDescriptorBit(1);
		PutDescriptorBit(distance & (1 << 10));
		PutDescriptorBit(distance & (1 << 9));
		PutDescriptorBit(distance & (1 << 8));
		PutMatchByte(distance & 0xFF);
		PutDescriptorBit(length == 5);
		PutDescriptorBit(length == 4);
	}
	else //if (length >= 6)
	{
		PutDescriptorBit(0);
		PutDescriptorBit(1);
		PutDescriptorBit(distance & (1 << 10));
		PutDescriptorBit(distance & (1 << 9));
		PutDescriptorBit(distance & (1 << 8));
		PutMatchByte(distance & 0xFF);
		PutDescriptorBit(1);
		PutDescriptorBit(1);
		PutMatchByte(length);
	}
}

static unsigned int GetMatchCost(size_t distance, size_t length, void *user)
{
	(void)user;

	if (length >= 2 && length <= 3 && distance < 256)
		return 2 + 8 + 1;		// Descriptor bits, offset byte, length bit
	else if (length >= 3 && length <= 5)
		return 2 + 3 + 8 + 2;		// Descriptor bits, offset bits, offset byte, length bits
	else if (length >= 6)
		return 2 + 3 + 8 + 2 + 8;	// Descriptor bits, offset bits, offset byte, (blank) length bits, length byte
	else
		return 0; 			// In the event a match cannot be compressed
}

static void FindExtraMatches(unsigned char *data, size_t data_size, size_t offset, ClownLZSS_GraphEdge *node_meta_array, void *user)
{
	(void)data;
	(void)data_size;
	(void)offset;
	(void)node_meta_array;
	(void)user;
}

static CLOWNLZSS_MAKE_COMPRESSION_FUNCTION(CompressData, unsigned char, 0xFF, 0x7FF, FindExtraMatches, 1 + 8, DoLiteral, GetMatchCost, DoMatch)

static void ChameleonCompressStream(unsigned char *data, size_t data_size, MemoryStream *output_stream, void *user_data)
{
	(void)user_data;

	match_stream = MemoryStream_Create(0x100, true);
	descriptor_stream = MemoryStream_Create(0x100, true);
	descriptor_bits_remaining = TOTAL_DESCRIPTOR_BITS;

	CompressData(data, data_size, NULL);

	// Terminator match
	PutDescriptorBit(0);
	PutDescriptorBit(1);
	PutDescriptorBit(0);
	PutDescriptorBit(0);
	PutDescriptorBit(0);
	PutMatchByte(0);
	PutDescriptorBit(1);
	PutDescriptorBit(1);
	PutMatchByte(0);

	MemoryStream_WriteByte(descriptor_stream, descriptor << descriptor_bits_remaining);

	const size_t descriptor_buffer_size = MemoryStream_GetPosition(descriptor_stream);
	unsigned char *descriptor_buffer = MemoryStream_GetBuffer(descriptor_stream);

	MemoryStream_WriteByte(output_stream, descriptor_buffer_size >> 8);
	MemoryStream_WriteByte(output_stream, descriptor_buffer_size & 0xFF);

	MemoryStream_WriteBytes(output_stream, descriptor_buffer, descriptor_buffer_size);

	const size_t match_buffer_size = MemoryStream_GetPosition(match_stream);
	unsigned char *match_buffer = MemoryStream_GetBuffer(match_stream);

	MemoryStream_WriteBytes(output_stream, match_buffer, match_buffer_size);

	MemoryStream_Destroy(descriptor_stream);
	MemoryStream_Destroy(match_stream);
}

unsigned char* ChameleonCompress(unsigned char *data, size_t data_size, size_t *compressed_size)
{
	return RegularWrapper(data, data_size, compressed_size, NULL, ChameleonCompressStream);
}

unsigned char* ModuledChameleonCompress(unsigned char *data, size_t data_size, size_t *compressed_size, size_t module_size)
{
	return ModuledCompressionWrapper(data, data_size, compressed_size, NULL, ChameleonCompressStream, module_size, 1);
}
