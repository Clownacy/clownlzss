/*
	(C) 2018-2019 Clownacy

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

#pragma once

#include <limits.h>
#include <stddef.h>
#include <stdlib.h>

#define CLOWNLZSS_MIN(a, b) (a) < (b) ? (a) : (b)
#define CLOWNLZSS_MAX(a, b) (a) > (b) ? (a) : (b)

typedef struct ClownLZSS_GraphEdge
{
	union
	{
		unsigned int cost;
		size_t next_node_index;
	} u;
	size_t previous_node_index;
	size_t match_length;
	size_t match_offset;
} ClownLZSS_GraphEdge;

#define CLOWNLZSS_MAKE_COMPRESSION_FUNCTION(NAME, TYPE, MAX_MATCH_LENGTH, MAX_MATCH_DISTANCE, FIND_EXTRA_MATCHES, LITERAL_COST, LITERAL_CALLBACK, MATCH_COST_CALLBACK, MATCH_CALLBACK)\
void NAME(TYPE *data, size_t data_size, void *user)\
{\
	ClownLZSS_GraphEdge *node_meta_array = (ClownLZSS_GraphEdge*)malloc((data_size + 1) * sizeof(ClownLZSS_GraphEdge));	/* +1 for the end-node */\
\
	/* Set costs to maximum possible value, so later comparisons work */\
	node_meta_array[0].u.cost = 0;\
	for (size_t i = 1; i < data_size + 1; ++i)\
		node_meta_array[i].u.cost = UINT_MAX;\
\
	/* Search for matches, to populate the edges of the LZSS graph.
	   Notably, while doing this, we're also using a shortest-path
	   algorithm on the edges to find the best combination of matches
	   to produce the smallest file. */\
	for (size_t i = 0; i < data_size; ++i)\
	{\
		const size_t max_read_ahead = CLOWNLZSS_MIN(MAX_MATCH_LENGTH, data_size - i);\
		const size_t max_read_behind = MAX_MATCH_DISTANCE > i ? 0 : i - MAX_MATCH_DISTANCE;\
\
		FIND_EXTRA_MATCHES(data, data_size, i, node_meta_array, user);\
\
		for (size_t j = i; j-- > max_read_behind;)\
		{\
			for (size_t k = 0; k < max_read_ahead; ++k)\
			{\
				if (data[i + k] == data[j + k])\
				{\
					const unsigned int cost = MATCH_COST_CALLBACK(i - j, k + 1, user);\
\
					if (cost && node_meta_array[i + k + 1].u.cost > node_meta_array[i].u.cost + cost)\
					{\
						node_meta_array[i + k + 1].u.cost = node_meta_array[i].u.cost + cost;\
						node_meta_array[i + k + 1].previous_node_index = i;\
						node_meta_array[i + k + 1].match_length = k + 1;\
						node_meta_array[i + k + 1].match_offset = j;\
					}\
				}\
				else\
					break;\
			}\
		}\
\
		/* Insert a literal match if it's more efficient */\
		if (node_meta_array[i + 1].u.cost >= node_meta_array[i].u.cost + LITERAL_COST)\
		{\
			node_meta_array[i + 1].u.cost = node_meta_array[i].u.cost + LITERAL_COST;\
			node_meta_array[i + 1].previous_node_index = i;\
			node_meta_array[i + 1].match_length = 0;\
		}\
	}\
\
	/* Mark start/end nodes for the following loops */\
	node_meta_array[0].previous_node_index = (size_t)-1;\
	node_meta_array[data_size].u.next_node_index = (size_t)-1;\
\
	/* Reverse the direction of the edges, so we can parse the LZSS graph from start to end */\
	for (size_t node_index = data_size; node_meta_array[node_index].previous_node_index != (size_t)-1; node_index = node_meta_array[node_index].previous_node_index)\
		node_meta_array[node_meta_array[node_index].previous_node_index].u.next_node_index = node_index;\
\
	/* Go through our now-complete LZSS graph, and output the optimally-compressed file */\
	for (size_t node_index = 0; node_meta_array[node_index].u.next_node_index != (size_t)-1; node_index = node_meta_array[node_index].u.next_node_index)\
	{\
		const size_t next_index = node_meta_array[node_index].u.next_node_index;\
		const size_t length = node_meta_array[next_index].match_length;\
		const size_t offset = node_meta_array[next_index].match_offset;\
\
		if (length == 0)\
			LITERAL_CALLBACK(data[node_index], user);\
		else\
			MATCH_CALLBACK(next_index - length - offset, length, offset, user);\
	}\
\
	free(node_meta_array);\
}
