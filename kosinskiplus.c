#include "kosinskiplus.h"

#ifndef __cplusplus
#include <stdbool.h>
#endif
#include <stddef.h>

#include "clownlzss.h"
#include "common.h"
#include "memory_stream.h"

#define TOTAL_DESCRIPTOR_BITS 8

typedef struct KosinskiPlusInstance
{
	MemoryStream *output_stream;
	MemoryStream *match_stream;

	unsigned char descriptor;
	unsigned int descriptor_bits_remaining;
} KosinskiPlusInstance;

static void FlushData(KosinskiPlusInstance *instance)
{
	MemoryStream_WriteByte(instance->output_stream, instance->descriptor);

	const size_t match_buffer_size = MemoryStream_GetPosition(instance->match_stream);
	unsigned char *match_buffer = MemoryStream_GetBuffer(instance->match_stream);

	MemoryStream_WriteBytes(instance->output_stream, match_buffer, match_buffer_size);
}

static void PutMatchByte(KosinskiPlusInstance *instance, unsigned char byte)
{
	MemoryStream_WriteByte(instance->match_stream, byte);
}

static void PutDescriptorBit(KosinskiPlusInstance *instance, bool bit)
{
	if (instance->descriptor_bits_remaining == 0)
	{
		FlushData(instance);

		instance->descriptor_bits_remaining = TOTAL_DESCRIPTOR_BITS;
		MemoryStream_Rewind(instance->match_stream);
	}

	--instance->descriptor_bits_remaining;

	instance->descriptor <<= 1;

	instance->descriptor |= bit;
}

static void DoLiteral(unsigned char value, void *user)
{
	KosinskiPlusInstance *instance = (KosinskiPlusInstance*)user;

	PutDescriptorBit(instance, 1);
	PutMatchByte(instance, value);
}

static void DoMatch(size_t distance, size_t length, size_t offset, void *user)
{
	(void)offset;

	KosinskiPlusInstance *instance = (KosinskiPlusInstance*)user;

	if (length >= 2 && length <= 5 && distance <= 256)
	{
		PutDescriptorBit(instance, 0);
		PutDescriptorBit(instance, 0);
		PutMatchByte(instance, (unsigned char)-distance);
		PutDescriptorBit(instance, (length - 2) & 2);
		PutDescriptorBit(instance, (length - 2) & 1);
	}
	else if (length >= 3 && length <= 9)
	{
		PutDescriptorBit(instance, 0);
		PutDescriptorBit(instance, 1);
		PutMatchByte(instance, ((-distance >> (8 - 3)) & 0xF8) | ((10 - length) & 7));
		PutMatchByte(instance, -distance & 0xFF);
	}
	else //if (length >= 10)
	{
		PutDescriptorBit(instance, 0);
		PutDescriptorBit(instance, 1);
		PutMatchByte(instance, (-distance >> (8 - 3)) & 0xF8);
		PutMatchByte(instance, -distance & 0xFF);
		PutMatchByte(instance, (unsigned char)(length - 9));
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

static void FindExtraMatches(unsigned char *data, size_t data_size, size_t offset, ClownLZSS_GraphEdge *node_meta_array, void *user)
{
	(void)data;
	(void)data_size;
	(void)offset;
	(void)node_meta_array;
	(void)user;
}

static CLOWNLZSS_MAKE_COMPRESSION_FUNCTION(CompressData, unsigned char, 0x100 + 8, 0x2000, FindExtraMatches, 1 + 8, DoLiteral, GetMatchCost, DoMatch)

static void KosinskiPlusCompressStream(unsigned char *data, size_t data_size, MemoryStream *output_stream, void *user)
{
	(void)user;

	KosinskiPlusInstance instance;
	instance.output_stream = output_stream;
	instance.match_stream = MemoryStream_Create(0x10, true);
	instance.descriptor_bits_remaining = TOTAL_DESCRIPTOR_BITS;

	CompressData(data, data_size, &instance);

	// Terminator match
	PutDescriptorBit(&instance, 0);
	PutDescriptorBit(&instance, 1);
	PutMatchByte(&instance, 0xF0);
	PutMatchByte(&instance, 0x00);
	PutMatchByte(&instance, 0x00);

	instance.descriptor <<= instance.descriptor_bits_remaining;
	FlushData(&instance);

	MemoryStream_Destroy(instance.match_stream);
}

unsigned char* ClownLZSS_KosinskiPlusCompress(unsigned char *data, size_t data_size, size_t *compressed_size)
{
	return RegularWrapper(data, data_size, compressed_size, NULL, KosinskiPlusCompressStream);
}

unsigned char* ClownLZSS_ModuledKosinskiPlusCompress(unsigned char *data, size_t data_size, size_t *compressed_size, size_t module_size)
{
	return ModuledCompressionWrapper(data, data_size, compressed_size, NULL, KosinskiPlusCompressStream, module_size, 1);
}
