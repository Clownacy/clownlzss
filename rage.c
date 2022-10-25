/*
	(C) 2020-2022 Clownacy

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

/* This is the worst thing I've ever written - it uses clownlzss to compress
   files in Streets of Rage 2's 'Rage' format... which is RLE. */

#include "rage.h"

#include <stddef.h>

#include "clownlzss.h"
#include "common.h"

#define TOTAL_DESCRIPTOR_BITS 8

static size_t GetMatchCost(size_t distance, size_t length, void *user)
{
	(void)distance;
	(void)length;
	(void)user;

	if (length >= 4)
		return (2 + (((length - 4 - 3) + (0x1F - 1)) / 0x1F)) * 8;
	else
		return 0;
}

static void FindExtraMatches(const unsigned char *data, size_t data_size, size_t offset, ClownLZSS_GraphEdge *node_meta_array, void *user)
{
	size_t max_read_ahead;
	size_t k;

	(void)user;

	/* Look for RLE-matches */
	max_read_ahead = CLOWNLZSS_MIN(0xFFF + 4, data_size - offset);

	for (k = 0; k < max_read_ahead; ++k)
	{
		if (data[offset + k] == data[offset])
		{
			unsigned int cost;

			if (k + 1 < 4)
				cost = 0;
			else
				cost = ((k + 1 - 4 > 0xF ? 2 : 1) + 1) * 8;

			if (cost && node_meta_array[offset + k + 1].u.cost > node_meta_array[offset].u.cost + cost)
			{
				node_meta_array[offset + k + 1].u.cost = node_meta_array[offset].u.cost + cost;
				node_meta_array[offset + k + 1].previous_node_index = offset;
				node_meta_array[offset + k + 1].match_offset = 0xFFFFFF00 | data[offset];	/* Horrible hack, like the rest of this compressor */
			}
		}
		else
			break;
	}

	/* Add uncompressed runs */
	max_read_ahead = CLOWNLZSS_MIN(0x1FFF, data_size - offset);

	for (k = 0; k < max_read_ahead; ++k)
	{
		const unsigned int cost = (k + 1 + (k + 1 > 0x1F ? 2 : 1)) * 8;

		if (cost && node_meta_array[offset + k + 1].u.cost > node_meta_array[offset].u.cost + cost)
		{
			node_meta_array[offset + k + 1].u.cost = node_meta_array[offset].u.cost + cost;
			node_meta_array[offset + k + 1].previous_node_index = offset;
			node_meta_array[offset + k + 1].match_offset = offset;	/* Points at itself */
		}
	}
}

/* TODO - Shouldn't the distance limit be 0x2000? */
static CLOWNLZSS_MAKE_COMPRESSION_FUNCTION(CompressData, 1, 0xFFFFFFFF/*dictionary-matches can be infinite*/, 0x1FFF, FindExtraMatches, 0xFFFFFFF/*dummy*/, GetMatchCost)

cc_bool ClownLZSS_RageCompress(const unsigned char *data, size_t data_size, const ClownLZSS_Callbacks *callbacks)
{
	ClownLZSS_Match *matches;
	size_t total_matches;
	size_t header_position, end_position;
	size_t i;

	/* Produce a series of LZSS compression matches. */
	if (!CompressData(data, data_size, &matches, &total_matches, NULL))
		return cc_false;

	/* Track the location of the header... */
	header_position = callbacks->tell(callbacks->user_data);

	/* ...and insert a placeholder there. */
	callbacks->write(callbacks->user_data, 0);
	callbacks->write(callbacks->user_data, 0);

	/* Produce Rage-formatted data. */
	for (i = 0; i < total_matches; ++i)
	{
		const size_t distance = matches[i].destination - matches[i].source;
		const size_t offset = matches[i].source;

		size_t length;

		length = matches[i].length;

		if (distance == 0)
		{
			size_t i;

			/* Uncompressed run */
			if (length > 0x1F)
			{
				callbacks->write(callbacks->user_data, 0x20 | ((length >> 8) & 0x1F));
				callbacks->write(callbacks->user_data, length & 0xFF);
			}
			else
			{
				callbacks->write(callbacks->user_data, length);
			}

			for (i = 0; i < length; ++i)
				callbacks->write(callbacks->user_data, data[offset + i]);
		}
		else if ((offset & 0xFFFFFF00) == 0xFFFFFF00)
		{
			/* RLE-match */
			length -= 4;

			if (length > 0xF)
			{
				callbacks->write(callbacks->user_data, 0x40 | 0x10 | ((length >> 8) & 0xF));
				callbacks->write(callbacks->user_data, length & 0xFF);
			}
			else
			{
				callbacks->write(callbacks->user_data, 0x40 | (length & 0xF));
			}

			callbacks->write(callbacks->user_data, offset & 0xFF);
		}
		else
		{
			size_t thing;

			/* Dictionary-match */
			length -= 4;

			/* The first match can only encode 7 bytes */
			thing = length > 3 ? 3 : length;

			callbacks->write(callbacks->user_data, 0x80 | (thing << 5) | ((distance >> 8) & 0x1F));
			callbacks->write(callbacks->user_data, distance & 0xFF);

			length -= thing;

			/* If there are still more bytes in this match, do them in blocks of 0x1F bytes */
			while (length != 0)
			{
				thing = length > 0x1F ? 0x1F : length;

				callbacks->write(callbacks->user_data, 0x60 | thing);
				length -= thing;
			}
		}
	}

	/* We don't need the matches anymore. */
	free(matches);

	/* Grab the current position for later. */
	end_position = callbacks->tell(callbacks->user_data);

	/* Rewind to the header... */
	callbacks->seek(callbacks->user_data, header_position);

	/* ...and complete it. */
	callbacks->write(callbacks->user_data, ((end_position - header_position) >> (8 * 0)) & 0xFF);
	callbacks->write(callbacks->user_data, ((end_position - header_position) >> (8 * 1)) & 0xFF);

	/* Seek back to the end of the file just as the caller might expect us to do. */
	callbacks->seek(callbacks->user_data, end_position);

	return cc_true;
}
