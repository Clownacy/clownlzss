/*
	(C) 2018-2021 Clownacy

	This software is provided 'as-is', without any express or implied
	warranty.  In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not
	   claim that you wrote the original software. If you use this software
	   in a product, an acknowledgment in the product documentation would be
	   appreciated but is not required.
	2. Altered source versions must be plainly marked as such, and must not be
	   misrepresented as being the original software.
	3. This notice may not be removed or altered from any source distribution.
*/

#include "chameleon.h"

#include <assert.h>
#include <stddef.h>

#include "clowncommon.h"

#include "clownlzss.h"
#include "common.h"
#include "memory_stream.h"

#define TOTAL_DESCRIPTOR_BITS 8

typedef struct ChameleonInstance
{
	MemoryStream match_stream;
	MemoryStream descriptor_stream;

	unsigned int descriptor;
	unsigned int descriptor_bits_remaining;
} ChameleonInstance;

static void PutMatchByte(ChameleonInstance *instance, unsigned int byte)
{
	MemoryStream_WriteByte(&instance->match_stream, byte);
}

static void PutDescriptorBit(ChameleonInstance *instance, cc_bool bit)
{
	assert(bit == 0 || bit == 1);

	if (instance->descriptor_bits_remaining == 0)
	{
		MemoryStream_WriteByte(&instance->descriptor_stream, instance->descriptor & 0xFF);

		instance->descriptor_bits_remaining = TOTAL_DESCRIPTOR_BITS;
	}

	--instance->descriptor_bits_remaining;

	instance->descriptor <<= 1;

	instance->descriptor |= bit;
}

static void DoLiteral(const unsigned char *value, void *user)
{
	ChameleonInstance *instance = (ChameleonInstance*)user;

	PutDescriptorBit(instance, 1);
	PutMatchByte(instance, value[0]);
}

static void DoMatch(size_t distance, size_t length, size_t offset, void *user)
{
	ChameleonInstance *instance = (ChameleonInstance*)user;

	(void)offset;

	if (length >= 2 && length <= 3 && distance < 0x100)
	{
		PutDescriptorBit(instance, 0);
		PutDescriptorBit(instance, 0);
		PutMatchByte(instance, distance);
		PutDescriptorBit(instance, length == 3);
	}
	else if (length >= 3 && length <= 5)
	{
		PutDescriptorBit(instance, 0);
		PutDescriptorBit(instance, 1);
		PutDescriptorBit(instance, !!(distance & (1 << 10)));
		PutDescriptorBit(instance, !!(distance & (1 << 9)));
		PutDescriptorBit(instance, !!(distance & (1 << 8)));
		PutMatchByte(instance, distance & 0xFF);
		PutDescriptorBit(instance, length == 5);
		PutDescriptorBit(instance, length == 4);
	}
	else /*if (length >= 6)*/
	{
		PutDescriptorBit(instance, 0);
		PutDescriptorBit(instance, 1);
		PutDescriptorBit(instance, !!(distance & (1 << 10)));
		PutDescriptorBit(instance, !!(distance & (1 << 9)));
		PutDescriptorBit(instance, !!(distance & (1 << 8)));
		PutMatchByte(instance, distance & 0xFF);
		PutDescriptorBit(instance, 1);
		PutDescriptorBit(instance, 1);
		PutMatchByte(instance, length);
	}
}

static size_t GetMatchCost(size_t distance, size_t length, void *user)
{
	(void)user;

	if (length >= 2 && length <= 3 && distance < 0x100)
		return 2 + 8 + 1;         /* Descriptor bits, offset byte, length bit */
	else if (length >= 3 && length <= 5)
		return 2 + 3 + 8 + 2;     /* Descriptor bits, offset bits, offset byte, length bits */
	else if (length >= 6)
		return 2 + 3 + 8 + 2 + 8; /* Descriptor bits, offset bits, offset byte, (blank) length bits, length byte */
	else
		return 0;                 /* In the event a match cannot be compressed */
}

static void FindExtraMatches(const unsigned char *data, size_t data_size, size_t offset, ClownLZSS_GraphEdge *node_meta_array, void *user)
{
	(void)data;
	(void)data_size;
	(void)offset;
	(void)node_meta_array;
	(void)user;
}

/* TODO - Shouldn't the length limit be 0x100, and the distance limit be 0x800? */
static CLOWNLZSS_MAKE_COMPRESSION_FUNCTION(CompressData, 1, 0xFF, 0x7FF, FindExtraMatches, 1 + 8, DoLiteral, GetMatchCost, DoMatch)

static void ChameleonCompressStream(const unsigned char *data, size_t data_size, MemoryStream *output_stream, void *user)
{
	ChameleonInstance instance;

	size_t descriptor_buffer_size;
	unsigned char *descriptor_buffer;

	size_t match_buffer_size;
	unsigned char *match_buffer;

	(void)user;

	MemoryStream_Create(&instance.match_stream, cc_true);
	MemoryStream_Create(&instance.descriptor_stream, cc_true);
	instance.descriptor = 0;
	instance.descriptor_bits_remaining = TOTAL_DESCRIPTOR_BITS;

	CompressData(data, data_size, &instance);

	/* Terminator match */
	PutDescriptorBit(&instance, 0);
	PutDescriptorBit(&instance, 1);
	PutDescriptorBit(&instance, 0);
	PutDescriptorBit(&instance, 0);
	PutDescriptorBit(&instance, 0);
	PutMatchByte(&instance, 0);
	PutDescriptorBit(&instance, 1);
	PutDescriptorBit(&instance, 1);
	PutMatchByte(&instance, 0);

	MemoryStream_WriteByte(&instance.descriptor_stream, (instance.descriptor << instance.descriptor_bits_remaining) & 0xFF);

	descriptor_buffer_size = MemoryStream_GetPosition(&instance.descriptor_stream);
	descriptor_buffer = MemoryStream_GetBuffer(&instance.descriptor_stream);

	MemoryStream_WriteByte(output_stream, (descriptor_buffer_size >> 8) & 0xFF);
	MemoryStream_WriteByte(output_stream, (descriptor_buffer_size >> 0) & 0xFF);

	MemoryStream_Write(output_stream, descriptor_buffer, 1, descriptor_buffer_size);

	match_buffer_size = MemoryStream_GetPosition(&instance.match_stream);
	match_buffer = MemoryStream_GetBuffer(&instance.match_stream);

	MemoryStream_Write(output_stream, match_buffer, 1, match_buffer_size);

	MemoryStream_Destroy(&instance.descriptor_stream);
	MemoryStream_Destroy(&instance.match_stream);
}

unsigned char* ClownLZSS_ChameleonCompress(const unsigned char *data, size_t data_size, size_t *compressed_size)
{
	return RegularWrapper(data, data_size, compressed_size, NULL, ChameleonCompressStream);
}

unsigned char* ClownLZSS_ModuledChameleonCompress(const unsigned char *data, size_t data_size, size_t *compressed_size, size_t module_size)
{
	return ModuledCompressionWrapper(data, data_size, compressed_size, NULL, ChameleonCompressStream, module_size, 1);
}
