#include <limits.h>
#include <stddef.h>
#include <stdlib.h>

#undef MIN
#define MIN(a, b) (a) < (b) ? (a) : (b)

typedef struct NodeMeta
{
	union
	{
		unsigned int cost;
		size_t next_node_index;
	};
	size_t previous_node_index;
	size_t match_distance;
	size_t match_length;
} NodeMeta;

static NodeMeta *node_meta_array;

void ClownLZSS_CompressData(unsigned char *data, size_t data_size, size_t max_match_length, size_t max_match_distance, size_t literal_cost, unsigned int (*cost_callback)(size_t distance, size_t length), void (*literal_callback)(unsigned char value), void (*match_callback)(size_t longest_match_index, size_t longest_match_length))
{
	node_meta_array = malloc((data_size + 1) * sizeof(NodeMeta));	// +1 for the end-node

	node_meta_array[0].cost = 0;
	for (size_t i = 1; i < data_size + 1; ++i)
		node_meta_array[i].cost = UINT_MAX;

	for (size_t i = 0; i < data_size; ++i)
	{
		const size_t max_read_ahead = MIN(max_match_length, data_size - i);
		const size_t max_read_behind = MIN(i, max_match_distance);

		for (size_t j = 1; j < max_read_behind + 1; ++j)
		{
			for (size_t k = 0; k < max_read_ahead; ++k)
			{
				if (data[i + k] == data[i - j + k])
				{
					const unsigned int cost = cost_callback(j, k + 1);

					if (cost && node_meta_array[i + k + 1].cost > node_meta_array[i].cost + cost)
					{
						node_meta_array[i + k + 1].cost = node_meta_array[i].cost + cost;
						node_meta_array[i + k + 1].previous_node_index = i;
						node_meta_array[i + k + 1].match_distance = j;
						node_meta_array[i + k + 1].match_length = k + 1;
					}
				}
				else
					break;
			}
		}

		if (node_meta_array[i + 1].cost > node_meta_array[i].cost + literal_cost)
		{
			node_meta_array[i + 1].cost = node_meta_array[i].cost + literal_cost;
			node_meta_array[i + 1].previous_node_index = i;
			node_meta_array[i + 1].match_distance = 0;
		}
	}

	node_meta_array[0].previous_node_index = UINT_MAX;
	node_meta_array[data_size].next_node_index = UINT_MAX;
	for (size_t node_index = data_size; node_meta_array[node_index].previous_node_index != UINT_MAX; node_index = node_meta_array[node_index].previous_node_index)
		node_meta_array[node_meta_array[node_index].previous_node_index].next_node_index = node_index;

	for (size_t node_index = 0; node_meta_array[node_index].next_node_index != UINT_MAX; node_index = node_meta_array[node_index].next_node_index)
	{
		const size_t next_index = node_meta_array[node_index].next_node_index;

		if (node_meta_array[next_index].match_distance)
			match_callback(node_meta_array[next_index].match_distance, node_meta_array[next_index].match_length);
		else
			literal_callback(data[node_index]);
	}

	free(node_meta_array);
}
