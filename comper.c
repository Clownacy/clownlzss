/*
	(C) 2018-2022 Clownacy

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

#include "comper.h"

#include <assert.h>
#include <stddef.h>

#include "clowncommon.h"

#include "clownlzss.h"
#include "common.h"

#define TOTAL_DESCRIPTOR_BITS 16

typedef struct ComperInstance
{
	const ClownLZSS_Callbacks *callbacks;

	size_t descriptor_position;

	unsigned int descriptor;
	unsigned int descriptor_bits_remaining;
} ComperInstance;

static size_t GetMatchCost(size_t distance, size_t length, void *user)
{
	(void)distance;
	(void)length;
	(void)user;

	return 1 + 16;	/* Descriptor bit, offset/length bytes */
}

static void FindExtraMatches(const unsigned char *data, size_t data_size, size_t offset, ClownLZSS_GraphEdge *node_meta_array, void *user)
{
	(void)data;
	(void)data_size;
	(void)offset;
	(void)node_meta_array;
	(void)user;
}

static CLOWNLZSS_MAKE_COMPRESSION_FUNCTION(CompressData, 2, 0x100, 0x100, FindExtraMatches, 1 + 16, GetMatchCost)

static void BeginDescriptorField(ComperInstance *instance)
{
	const ClownLZSS_Callbacks* const callbacks = instance->callbacks;

	/* Log the placement of the descriptor field. */
	instance->descriptor_position = callbacks->tell(callbacks->user_data);

	/* Insert a placeholder. */
	callbacks->write(callbacks->user_data, 0);
	callbacks->write(callbacks->user_data, 0);
}

static void FinishDescriptorField(ComperInstance *instance)
{
	const ClownLZSS_Callbacks* const callbacks = instance->callbacks;

	/* Back up current position. */
	const size_t current_position = callbacks->tell(callbacks->user_data);

	/* Go back to the descriptor field. */
	callbacks->seek(callbacks->user_data, instance->descriptor_position);

	/* Write the complete descriptor field. */
	callbacks->write(callbacks->user_data, (instance->descriptor >> (8 * 1)) & 0xFF);
	callbacks->write(callbacks->user_data, (instance->descriptor >> (8 * 0)) & 0xFF);

	/* Seek back to where we were before. */
	callbacks->seek(callbacks->user_data, current_position);
}

static void PutDescriptorBit(ComperInstance *instance, cc_bool bit)
{
	assert(bit == 0 || bit == 1);

	if (instance->descriptor_bits_remaining == 0)
	{
		instance->descriptor_bits_remaining = TOTAL_DESCRIPTOR_BITS;

		FinishDescriptorField(instance);
		BeginDescriptorField(instance);
	}

	--instance->descriptor_bits_remaining;

	instance->descriptor <<= 1;

	instance->descriptor |= bit;
}

cc_bool ClownLZSS_ComperCompress(const unsigned char *data, size_t data_size, const ClownLZSS_Callbacks *callbacks)
{
	ComperInstance instance;
	ClownLZSS_Match *matches;
	size_t total_matches;
	size_t i;

	/* Set up the state. */
	instance.callbacks = callbacks;
	instance.descriptor = 0;
	instance.descriptor_bits_remaining = TOTAL_DESCRIPTOR_BITS;

	/* Produce a series of LZSS compression matches. */
	if (!CompressData(data, data_size, &matches, &total_matches, &instance))
		return cc_false;

	/* Begin first descriptor field. */
	BeginDescriptorField(&instance);

	/* Produce Comper-formatted data. */
	for (i = 0; i < total_matches; ++i)
	{
		if (CLOWNLZSS_MATCH_IS_LITERAL(matches[i]))
		{
			PutDescriptorBit(&instance, 0);
			callbacks->write(callbacks->user_data, data[matches[i].destination * 2 + 0]);
			callbacks->write(callbacks->user_data, data[matches[i].destination * 2 + 1]);
		}
		else
		{
			const size_t distance = matches[i].destination - matches[i].source;
			const size_t length = matches[i].length;

			PutDescriptorBit(&instance, 1);
			callbacks->write(callbacks->user_data, -distance & 0xFF);
			callbacks->write(callbacks->user_data, length - 1);
		}
	}

	/* We don't need the matches anymore. */
	free(matches);

	/* Add the terminator match. */
	PutDescriptorBit(&instance, 1);
	callbacks->write(callbacks->user_data, 0);
	callbacks->write(callbacks->user_data, 0);

	/* The descriptor field may be incomplete, so move the bits into their proper place. */
	instance.descriptor <<= instance.descriptor_bits_remaining;

	/* Finish last descriptor field. */
	FinishDescriptorField(&instance);

	return cc_true;
}
