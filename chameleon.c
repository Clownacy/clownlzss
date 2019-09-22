#include "chameleon.h"

#ifndef __cplusplus
#include <stdbool.h>
#endif
#include <stddef.h>

#include "clownlzss.h"
#include "common.h"
#include "memory_stream.h"

#define TOTAL_DESCRIPTOR_BITS 8

typedef struct ChameleonInstance
{
	MemoryStream *match_stream;
	MemoryStream *descriptor_stream;

	unsigned char descriptor;
	unsigned int descriptor_bits_remaining;
} ChameleonInstance;

static void PutMatchByte(ChameleonInstance *instance, unsigned char byte)
{
	MemoryStream_WriteByte(instance->match_stream, byte);
}

static void PutDescriptorBit(ChameleonInstance *instance, bool bit)
{
	if (instance->descriptor_bits_remaining == 0)
	{
		MemoryStream_WriteByte(instance->descriptor_stream, instance->descriptor);

		instance->descriptor_bits_remaining = TOTAL_DESCRIPTOR_BITS;
	}

	--instance->descriptor_bits_remaining;

	instance->descriptor <<= 1;

	instance->descriptor |= bit;
}

static void DoLiteral(unsigned char value, void *user)
{
	ChameleonInstance *instance = (ChameleonInstance*)user;

	PutDescriptorBit(instance, 1);
	PutMatchByte(instance, value);
}

static void DoMatch(size_t distance, size_t length, size_t offset, void *user)
{
	(void)offset;

	ChameleonInstance *instance = (ChameleonInstance*)user;

	if (length >= 2 && length <= 3 && distance < 256)
	{
		PutDescriptorBit(instance, 0);
		PutDescriptorBit(instance, 0);
		PutMatchByte(instance, (unsigned char)distance);
		PutDescriptorBit(instance, length == 3);
	}
	else if (length >= 3 && length <= 5)
	{
		PutDescriptorBit(instance, 0);
		PutDescriptorBit(instance, 1);
		PutDescriptorBit(instance, distance & (1 << 10));
		PutDescriptorBit(instance, distance & (1 << 9));
		PutDescriptorBit(instance, distance & (1 << 8));
		PutMatchByte(instance, distance & 0xFF);
		PutDescriptorBit(instance, length == 5);
		PutDescriptorBit(instance, length == 4);
	}
	else //if (length >= 6)
	{
		PutDescriptorBit(instance, 0);
		PutDescriptorBit(instance, 1);
		PutDescriptorBit(instance, distance & (1 << 10));
		PutDescriptorBit(instance, distance & (1 << 9));
		PutDescriptorBit(instance, distance & (1 << 8));
		PutMatchByte(instance, distance & 0xFF);
		PutDescriptorBit(instance, 1);
		PutDescriptorBit(instance, 1);
		PutMatchByte(instance, (unsigned char)length);
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

static void ChameleonCompressStream(unsigned char *data, size_t data_size, MemoryStream *output_stream, void *user)
{
	(void)user;

	ChameleonInstance instance;
	instance.match_stream = MemoryStream_Create(true);
	instance.descriptor_stream = MemoryStream_Create(true);
	instance.descriptor_bits_remaining = TOTAL_DESCRIPTOR_BITS;

	CompressData(data, data_size, &instance);

	// Terminator match
	PutDescriptorBit(&instance, 0);
	PutDescriptorBit(&instance, 1);
	PutDescriptorBit(&instance, 0);
	PutDescriptorBit(&instance, 0);
	PutDescriptorBit(&instance, 0);
	PutMatchByte(&instance, 0);
	PutDescriptorBit(&instance, 1);
	PutDescriptorBit(&instance, 1);
	PutMatchByte(&instance, 0);

	MemoryStream_WriteByte(instance.descriptor_stream, instance.descriptor << instance.descriptor_bits_remaining);

	const size_t descriptor_buffer_size = MemoryStream_GetPosition(instance.descriptor_stream);
	unsigned char *descriptor_buffer = MemoryStream_GetBuffer(instance.descriptor_stream);

	MemoryStream_WriteByte(output_stream, (descriptor_buffer_size >> 8) & 0xFF);
	MemoryStream_WriteByte(output_stream, descriptor_buffer_size & 0xFF);

	MemoryStream_WriteBytes(output_stream, descriptor_buffer, descriptor_buffer_size);

	const size_t match_buffer_size = MemoryStream_GetPosition(instance.match_stream);
	unsigned char *match_buffer = MemoryStream_GetBuffer(instance.match_stream);

	MemoryStream_WriteBytes(output_stream, match_buffer, match_buffer_size);

	MemoryStream_Destroy(instance.descriptor_stream);
	MemoryStream_Destroy(instance.match_stream);
}

unsigned char* ClownLZSS_ChameleonCompress(unsigned char *data, size_t data_size, size_t *compressed_size)
{
	return RegularWrapper(data, data_size, compressed_size, NULL, ChameleonCompressStream);
}

unsigned char* ClownLZSS_ModuledChameleonCompress(unsigned char *data, size_t data_size, size_t *compressed_size, size_t module_size)
{
	return ModuledCompressionWrapper(data, data_size, compressed_size, NULL, ChameleonCompressStream, module_size, 1);
}
