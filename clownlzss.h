#pragma once

#include <limits.h>
#include <stddef.h>
#include <stdlib.h>

#define CLOWNLZSS_MIN(a, b) (a) < (b) ? (a) : (b)

typedef struct ClownLZSS_NodeMeta
{
	union
	{
		unsigned int cost;
		size_t next_node_index;
	};
	size_t previous_node_index;
	size_t match_distance;
	size_t match_length;
} ClownLZSS_NodeMeta;

#define CLOWNLZSS_MAKE_FUNCTION(NAME, TYPE, MAX_MATCH_LENGTH, MAX_MATCH_DISTANCE, LITERAL_COST, LITERAL_CALLBACK, MATCH_COST_CALLBACK, MATCH_CALLBACK)\
void NAME(TYPE *data, size_t data_size, void *user)\
{\
	ClownLZSS_NodeMeta *node_meta_array = malloc((data_size + 1) * sizeof(ClownLZSS_NodeMeta));	/* +1 for the end-node */\
\
	node_meta_array[0].cost = 0;\
	for (size_t i = 1; i < data_size + 1; ++i)\
		node_meta_array[i].cost = UINT_MAX;\
\
	for (size_t i = 0; i < data_size; ++i)\
	{\
		const size_t max_read_ahead = CLOWNLZSS_MIN(MAX_MATCH_LENGTH, data_size - i);\
		const size_t max_read_behind = CLOWNLZSS_MIN(i, MAX_MATCH_DISTANCE);\
\
		for (size_t j = 1; j < max_read_behind + 1; ++j)\
		{\
			for (size_t k = 0; k < max_read_ahead; ++k)\
			{\
				if (data[i + k] == data[i - j + k])\
				{\
					const unsigned int cost = MATCH_COST_CALLBACK(j, k + 1, user);\
\
					if (cost && node_meta_array[i + k + 1].cost > node_meta_array[i].cost + cost)\
					{\
						node_meta_array[i + k + 1].cost = node_meta_array[i].cost + cost;\
						node_meta_array[i + k + 1].previous_node_index = i;\
						node_meta_array[i + k + 1].match_distance = j;\
						node_meta_array[i + k + 1].match_length = k + 1;\
					}\
				}\
				else\
					break;\
			}\
		}\
\
		if (node_meta_array[i + 1].cost >= node_meta_array[i].cost + LITERAL_COST)\
		{\
			node_meta_array[i + 1].cost = node_meta_array[i].cost + LITERAL_COST;\
			node_meta_array[i + 1].previous_node_index = i;\
			node_meta_array[i + 1].match_distance = 0;\
		}\
	}\
\
	node_meta_array[0].previous_node_index = UINT_MAX;\
	node_meta_array[data_size].next_node_index = UINT_MAX;\
	for (size_t node_index = data_size; node_meta_array[node_index].previous_node_index != UINT_MAX; node_index = node_meta_array[node_index].previous_node_index)\
		node_meta_array[node_meta_array[node_index].previous_node_index].next_node_index = node_index;\
\
	for (size_t node_index = 0; node_meta_array[node_index].next_node_index != UINT_MAX; node_index = node_meta_array[node_index].next_node_index)\
	{\
		const size_t next_index = node_meta_array[node_index].next_node_index;\
\
		if (node_meta_array[next_index].match_distance)\
			MATCH_CALLBACK(node_meta_array[next_index].match_distance, node_meta_array[next_index].match_length, user);\
		else\
			LITERAL_CALLBACK(data[node_index], user);\
	}\
\
	free(node_meta_array);\
}
