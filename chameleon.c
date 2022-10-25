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

#include "chameleon.h"

#include <assert.h>
#include <stddef.h>

#include "clowncommon.h"

#include "clownlzss.h"
#include "common.h"

#define TOTAL_DESCRIPTOR_BITS 8

typedef struct ChameleonInstance
{
	const ClownLZSS_Callbacks *callbacks;

	unsigned int descriptor;
	unsigned int descriptor_bits_remaining;
} ChameleonInstance;

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
static CLOWNLZSS_MAKE_COMPRESSION_FUNCTION(CompressData, 1, 0xFF, 0x7FF, FindExtraMatches, 1 + 8, GetMatchCost)

static void PutDescriptorBit(ChameleonInstance *instance, cc_bool bit)
{
	const ClownLZSS_Callbacks* const callbacks = instance->callbacks;

	assert(bit == 0 || bit == 1);

	if (instance->descriptor_bits_remaining == 0)
	{
		callbacks->write(callbacks->user_data, instance->descriptor & 0xFF);

		instance->descriptor_bits_remaining = TOTAL_DESCRIPTOR_BITS;
	}

	--instance->descriptor_bits_remaining;

	instance->descriptor <<= 1;

	instance->descriptor |= bit;
}


cc_bool ClownLZSS_ChameleonCompress(const unsigned char *data, size_t data_size, const ClownLZSS_Callbacks *callbacks)
{
	ChameleonInstance instance;
	ClownLZSS_Match *matches;
	size_t total_matches;
	size_t header_position, current_position;
	size_t i;

	/* Set up the state. */
	instance.callbacks = callbacks;
	instance.descriptor = 0;
	instance.descriptor_bits_remaining = TOTAL_DESCRIPTOR_BITS;

	/* Produce a series of LZSS compression matches. */
	if (!CompressData(data, data_size, &matches, &total_matches, &instance))
		return cc_false;

	/* Track the location of the header... */
	header_position = callbacks->tell(callbacks->user_data);

	/* ...and insert a placeholder there. */
	callbacks->write(callbacks->user_data, 0);
	callbacks->write(callbacks->user_data, 0);

	/* Produce Faxman-formatted data. */
	/* Unlike many other LZSS formats, Chameleon stores the descriptor fields separately from the rest of the data. */
	/* Iterate over the compression matches, outputting just the descriptor fields. */
	for (i = 0; i < total_matches; ++i)
	{
		if (CLOWNLZSS_MATCH_IS_LITERAL(matches[i]))
		{
			PutDescriptorBit(&instance, 1);
		}
		else
		{
			const size_t distance = matches[i].destination - matches[i].source;
			const size_t length = matches[i].length;

			if (length >= 2 && length <= 3 && distance < 0x100)
			{
				PutDescriptorBit(&instance, 0);
				PutDescriptorBit(&instance, 0);
				PutDescriptorBit(&instance, length == 3);
			}
			else if (length >= 3 && length <= 5)
			{
				PutDescriptorBit(&instance, 0);
				PutDescriptorBit(&instance, 1);
				PutDescriptorBit(&instance, !!(distance & (1 << 10)));
				PutDescriptorBit(&instance, !!(distance & (1 << 9)));
				PutDescriptorBit(&instance, !!(distance & (1 << 8)));
				PutDescriptorBit(&instance, length == 5);
				PutDescriptorBit(&instance, length == 4);
			}
			else /*if (length >= 6)*/
			{
				PutDescriptorBit(&instance, 0);
				PutDescriptorBit(&instance, 1);
				PutDescriptorBit(&instance, !!(distance & (1 << 10)));
				PutDescriptorBit(&instance, !!(distance & (1 << 9)));
				PutDescriptorBit(&instance, !!(distance & (1 << 8)));
				PutDescriptorBit(&instance, 1);
				PutDescriptorBit(&instance, 1);
			}
		}
	}

	/* Add the terminator match. */
	PutDescriptorBit(&instance, 0);
	PutDescriptorBit(&instance, 1);
	PutDescriptorBit(&instance, 0);
	PutDescriptorBit(&instance, 0);
	PutDescriptorBit(&instance, 0);
	PutDescriptorBit(&instance, 1);
	PutDescriptorBit(&instance, 1);

	/* The descriptor field may be incomplete, so move the bits into their proper place. */
	instance.descriptor <<= instance.descriptor_bits_remaining;

	/* Write last descriptor field. */
	callbacks->write(callbacks->user_data, instance.descriptor);

	/* Chameleon's header contains the size of the descriptor fields end, so, now that we know that, let's fill it in. */
	current_position = callbacks->tell(callbacks->user_data);
	callbacks->seek(callbacks->user_data, header_position);
	callbacks->write(callbacks->user_data, ((current_position - header_position - 2) >> (8 * 1)) & 0xFF);
	callbacks->write(callbacks->user_data, ((current_position - header_position - 2) >> (8 * 0)) & 0xFF);
	callbacks->seek(callbacks->user_data, current_position);

	/* Iterate over the compression matches again, now outputting just the literals and offset/length pairs. */
	for (i = 0; i < total_matches; ++i)
	{
		if (CLOWNLZSS_MATCH_IS_LITERAL(matches[i]))
		{
			callbacks->write(callbacks->user_data, data[matches[i].destination]);
		}
		else
		{
			const size_t distance = matches[i].destination - matches[i].source;
			const size_t length = matches[i].length;

			if (length >= 2 && length <= 3 && distance < 0x100)
			{
				callbacks->write(callbacks->user_data, distance);
			}
			else if (length >= 3 && length <= 5)
			{
				callbacks->write(callbacks->user_data, distance & 0xFF);
			}
			else /*if (length >= 6)*/
			{
				callbacks->write(callbacks->user_data, distance & 0xFF);
				callbacks->write(callbacks->user_data, length);
			}
		}
	}

	/* We don't need the matches anymore. */
	free(matches);

	/* Add the terminator match. */
	callbacks->write(callbacks->user_data, 0);
	callbacks->write(callbacks->user_data, 0);

	return cc_true;
}
