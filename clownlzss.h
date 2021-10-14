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

#ifndef CLOWNLZSS_H
#define CLOWNLZSS_H

#include <limits.h>
#include <stddef.h>
#include <stdlib.h>

#define CLOWNLZSS_MIN(a, b) ((a) < (b) ? (a) : (b))
#define CLOWNLZSS_MAX(a, b) ((a) > (b) ? (a) : (b))

typedef struct ClownLZSS_GraphEdge
{
	union
	{
		size_t cost;
		size_t next_node_index;
	} u;
	size_t previous_node_index;
	size_t match_length;
	size_t match_offset;
} ClownLZSS_GraphEdge;

typedef struct ClownLZSS_StringListNode
{
	const unsigned char *bytes;
	struct ClownLZSS_StringListNode *prev;
	struct ClownLZSS_StringListNode *next;
} ClownLZSS_StringListNode;

#define CLOWNLZSS_MAKE_COMPRESSION_FUNCTION(NAME, BYTES_PER_VALUE, MAX_MATCH_LENGTH, MAX_MATCH_DISTANCE, FIND_EXTRA_MATCHES, LITERAL_COST, LITERAL_CALLBACK, MATCH_COST_CALLBACK, MATCH_CALLBACK)\
void NAME(unsigned char *data, size_t data_size, void *user)\
{\
	ClownLZSS_GraphEdge *node_meta_array;\
	size_t i;\
	ClownLZSS_StringListNode string_list_nodes[MAX_MATCH_DISTANCE];\
	ClownLZSS_StringListNode string_list_heads[0x100];\
\
	/* Initialise the string list heads */\
	for (i = 0; i < sizeof(string_list_heads) / sizeof(string_list_heads[0]); ++i)\
		string_list_heads[i].next = NULL;\
\
	/* Initialise the string list nodes */\
	for (i = 0; i < sizeof(string_list_nodes) / sizeof(string_list_nodes[0]); ++i)\
		string_list_nodes[i].prev = NULL;\
\
	data_size /= BYTES_PER_VALUE;\
\
	node_meta_array = (ClownLZSS_GraphEdge*)malloc((data_size + 1) * sizeof(ClownLZSS_GraphEdge));	/* +1 for the end-node */\
\
	/* Set costs to maximum possible value, so later comparisons work */\
	node_meta_array[0].u.cost = 0;\
	for (i = 1; i < data_size + 1; ++i)\
		node_meta_array[i].u.cost = UINT_MAX;\
\
	/* Search for matches, to populate the edges of the LZSS graph.
	   Notably, while doing this, we're also using a shortest-path
	   algorithm on the edges to find the best combination of matches
	   to produce the smallest file. */\
\
	/* Advance through the data one step at a time */\
	for (i = 0; i < data_size; ++i)\
	{\
		ClownLZSS_StringListNode *searched_string_list_node;\
\
		ClownLZSS_StringListNode *current_string_list_head = &string_list_heads[data[i * BYTES_PER_VALUE] & 0xFF];\
		ClownLZSS_StringListNode *current_string_list_node = &string_list_nodes[i % MAX_MATCH_DISTANCE];\
\
		FIND_EXTRA_MATCHES(data, data_size, i * BYTES_PER_VALUE, node_meta_array, user);\
\
		/* `current_string_list_head` points to a linked-list of strings in the LZSS sliding window that match at least
		   one byte with the current string: iterate over it and generate every possible match for this string */\
		for (searched_string_list_node = current_string_list_head->next; searched_string_list_node != NULL; searched_string_list_node = searched_string_list_node->next)\
		{\
			size_t j;\
\
			const unsigned char *current_bytes = &data[i * BYTES_PER_VALUE];\
			const unsigned char *searched_bytes = searched_string_list_node->bytes;\
\
			/* If `BYTES_PER_VALUE` is not 1, then we have to re-evaluate the first value, otherwise we can skip it */\
			for (j = BYTES_PER_VALUE == 1; j < CLOWNLZSS_MIN(MAX_MATCH_LENGTH, data_size - i); ++j)\
			{\
				unsigned int mismatch = 0;\
				unsigned int l;\
\
				for (l = 0; l < BYTES_PER_VALUE; ++l)\
					mismatch |= current_bytes[j * BYTES_PER_VALUE + l] != searched_bytes[j * BYTES_PER_VALUE + l];\
\
				if (!mismatch)\
				{\
					/* Figure out how much it costs to encode the current run */\
					const unsigned int cost = MATCH_COST_CALLBACK(current_bytes - searched_bytes, j + 1, user);\
\
					/* Figure out if the cost is lower than that of any other runs that end at the same value as this one */\
					if (cost && node_meta_array[i + j + 1].u.cost > node_meta_array[i].u.cost + cost)\
					{\
						/* Record this new best run in the graph edge assigned to the value at the end of the run */\
						node_meta_array[i + j + 1].u.cost = node_meta_array[i].u.cost + cost;\
						node_meta_array[i + j + 1].previous_node_index = i;\
						node_meta_array[i + j + 1].match_length = j + 1;\
						node_meta_array[i + j + 1].match_offset = (searched_bytes - data) / BYTES_PER_VALUE;\
					}\
				}\
				else\
				{\
					/* No match: give up on the current run and go back to searching backwards */\
					break;\
				}\
			}\
		}\
\
		/* If a literal match is more efficient than all runs assigned to this value, then use that instead */\
		if (node_meta_array[i + 1].u.cost >= node_meta_array[i].u.cost + LITERAL_COST)\
		{\
			node_meta_array[i + 1].u.cost = node_meta_array[i].u.cost + LITERAL_COST;\
			node_meta_array[i + 1].previous_node_index = i;\
			node_meta_array[i + 1].match_length = 0;\
		}\
\
		/* Replace the oldest string in the list with the new string, since it's about to be pushed out of the LZSS sliding window */\
\
		/* Detach the old node in this slot */\
		if (current_string_list_node->prev != NULL)\
		{\
			current_string_list_node->prev->next = current_string_list_node->next;\
\
			if (current_string_list_node->next != NULL)\
				current_string_list_node->next->prev = current_string_list_node->prev;\
		}\
\
		/* Replace the old node with this new one, and insert it at the start of its matching list */\
		current_string_list_node->bytes = &data[i * BYTES_PER_VALUE];\
		current_string_list_node->prev = current_string_list_head;\
		current_string_list_node->next = current_string_list_head->next;\
\
		if (current_string_list_head->next != NULL)\
			current_string_list_head->next->prev = current_string_list_node;\
\
		current_string_list_head->next = current_string_list_node;\
	}\
\
	/* At this point, the edges will have formed a shortest-path from the start to the end:
	   You just have to start at the last edge, and follow it backwards all the way to the start. */\
\
	/* Mark start/end nodes for the following loops */\
	node_meta_array[0].previous_node_index = (size_t)-1;\
	node_meta_array[data_size].u.next_node_index = (size_t)-1;\
\
	/* Reverse the direction of the edges, so we can parse the LZSS graph from start to end */\
	for (i = data_size; node_meta_array[i].previous_node_index != (size_t)-1; i = node_meta_array[i].previous_node_index)\
		node_meta_array[node_meta_array[i].previous_node_index].u.next_node_index = i;\
\
	/* Go through our now-complete LZSS graph, and output the optimally-compressed file */\
	for (i = 0; node_meta_array[i].u.next_node_index != (size_t)-1; i = node_meta_array[i].u.next_node_index)\
	{\
		const size_t next_index = node_meta_array[i].u.next_node_index;\
		const size_t length = node_meta_array[next_index].match_length;\
		const size_t offset = node_meta_array[next_index].match_offset;\
\
		if (length == 0)\
			LITERAL_CALLBACK(&data[i * BYTES_PER_VALUE], user);\
		else\
			MATCH_CALLBACK(next_index - length - offset, length, offset, user);\
	}\
\
	free(node_meta_array);\
}

#endif /* CLOWNLZSS_H */
