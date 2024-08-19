/*
Copyright (c) 2018-2024 Clownacy

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.
*/

#include "clownlzss.h"

#include <stddef.h>
#include <stdlib.h>

int ClownLZSS_FindOptimalMatches(
	const int filler_value,
	const size_t maximum_match_length,
	const size_t maximum_match_distance,
	void (* const extra_matches_callback)(const unsigned char *data, size_t total_values, size_t offset, ClownLZSS_GraphEdge *node_meta_array, void *user),
	const size_t literal_cost,
	size_t (* const match_cost_callback)(size_t distance, size_t length, void *user),
	const unsigned char* const data,
	const size_t bytes_per_value,
	const size_t total_values,
	ClownLZSS_Match** const _matches,
	size_t* const _total_matches,
	const void* const user
)
{
	int success;

	success = 0;

	/* Handle the edge-case where the data is empty. */
	if (total_values == 0)
	{
		*_matches = NULL;
		*_total_matches = 0;
		success = 1;
	}
	else
	{
		const size_t node_meta_array_length = total_values + 1; /* +1 for the end-node */
		const size_t string_list_buffer_length = maximum_match_distance * 2 + 0x100;
		ClownLZSS_GraphEdge* const node_meta_array = (ClownLZSS_GraphEdge*)malloc(node_meta_array_length * sizeof(ClownLZSS_GraphEdge) + string_list_buffer_length * sizeof(size_t));

		if (node_meta_array != NULL)
		{
			size_t i;
			ClownLZSS_Match *matches;
			size_t total_matches;

			size_t* const prev = (size_t*)&node_meta_array[node_meta_array_length];
			size_t* const next = &prev[maximum_match_distance];
			const size_t DUMMY = -1;

			/* Initialise the string list heads */
			for (i = 0; i < 0x100; ++i)
				next[maximum_match_distance + i] = DUMMY;

			if (filler_value == -1)
			{
				/* Initialise the string list nodes */
				for (i = 0; i < maximum_match_distance; ++i)
					prev[i] = DUMMY;
			}
			else
			{
				next[maximum_match_distance + filler_value] = maximum_match_distance - 1;
				next[0] = DUMMY;
				prev[maximum_match_distance - 1] = maximum_match_distance + filler_value;

				/* Initialise the string list nodes */
				for (i = 0; i < maximum_match_distance - 1; ++i)
				{
					next[i + 1] = i;
					prev[i] = i + 1;
				}
			}

			/* Set costs to maximum possible value, so later comparisons work */
			node_meta_array[0].u.cost = 0;
			for (i = 1; i < total_values + 1; ++i)
				node_meta_array[i].u.cost = DUMMY;

			/* Search for matches, to populate the edges of the LZSS graph.
			   Notably, while doing this, we're also using a shortest-path
			   algorithm on the edges to find the best combination of matches
			   to produce the smallest file. */

			/* Advance through the data one step at a time */
			for (i = 0; i < total_values; ++i)
			{
				size_t match_string;

				const size_t string_list_head = maximum_match_distance + data[i * bytes_per_value];
				const size_t current_string = i % maximum_match_distance;

				if (extra_matches_callback != NULL)
					extra_matches_callback(data, total_values, i, node_meta_array, (void*)user);

				/* `string_list_head` points to a linked-list of strings in the LZSS sliding window that match at least
				   one byte with the current string: iterate over it and generate every possible match for this string */
				for (match_string = next[string_list_head]; match_string != DUMMY; match_string = next[match_string])
				{
					size_t j;

					const size_t distance = ((maximum_match_distance + i - match_string - 1) % maximum_match_distance) + 1;
					/* If `BYTES_PER_VALUE` is not 1, then we have to re-evaluate the first value, otherwise we can skip it */
					const unsigned int start = bytes_per_value == 1;
					const unsigned char *current_bytes = &data[(i + start) * bytes_per_value];
					const unsigned char *match_bytes = current_bytes - distance * bytes_per_value;

					for (j = start; j < CLOWNLZSS_MIN(maximum_match_length, total_values - i); ++j)
					{
						size_t l;

						if (match_bytes < data)
						{
							for (l = 0; l < bytes_per_value; ++l)
							{
								const unsigned char current_byte = *current_bytes;
								const unsigned char match_byte = (unsigned char)filler_value;

								++current_bytes;

								if (current_byte != match_byte)
									break;
							}

							match_bytes += bytes_per_value;
						}
						else
						{
							for (l = 0; l < bytes_per_value; ++l)
							{
								const unsigned char current_byte = *current_bytes;
								const unsigned char match_byte = *match_bytes;

								++current_bytes;
								++match_bytes;

								if (current_byte != match_byte)
									break;
							}
						}

						if (l != bytes_per_value)
						{
							/* No match: give up on the current run */
							break;
						}
						else
						{
							/* Figure out how much it costs to encode the current run */
							const size_t cost = match_cost_callback(distance, j + 1, (void*)user);

							/* Figure out if the cost is lower than that of any other runs that end at the same value as this one */
							if (cost != 0 && node_meta_array[i + j + 1].u.cost > node_meta_array[i].u.cost + cost)
							{
								/* Record this new best run in the graph edge assigned to the value at the end of the run */
								node_meta_array[i + j + 1].u.cost = node_meta_array[i].u.cost + cost;
								node_meta_array[i + j + 1].previous_node_index = i;
								node_meta_array[i + j + 1].match_offset = i - distance;
							}
						}
					}
				}

				/* If a literal match is more efficient than all runs assigned to this value, then use that instead */
				if (node_meta_array[i + 1].u.cost >= node_meta_array[i].u.cost + literal_cost)
				{
					node_meta_array[i + 1].u.cost = node_meta_array[i].u.cost + literal_cost;
					node_meta_array[i + 1].previous_node_index = i;
					node_meta_array[i + 1].match_offset = i + 1;
				}

				/* Replace the oldest string in the list with the new string, since it's about to be pushed out of the LZSS sliding window */

				/* Detach the old node in this slot */
				if (prev[current_string] != DUMMY)
					next[prev[current_string]] = DUMMY;

				/* Replace the old node with this new one, and insert it at the start of its matching list */
				prev[current_string] = string_list_head;
				next[current_string] = next[string_list_head];

				if (next[current_string] != DUMMY)
					prev[next[current_string]] = current_string;

				next[string_list_head] = current_string;
			}

			/* At this point, the edges will have formed a shortest-path from the start to the end:
			   You just have to start at the last edge, and follow it backwards all the way to the start. */

			/* Mark start/end nodes for the following loops */
			node_meta_array[0].previous_node_index = DUMMY;
			node_meta_array[total_values].u.next_node_index = DUMMY;

			/* Reverse the direction of the edges, so we can parse the LZSS graph from start to end */
			for (i = total_values; node_meta_array[i].previous_node_index != DUMMY; i = node_meta_array[i].previous_node_index)
				node_meta_array[node_meta_array[i].previous_node_index].u.next_node_index = i;

			/* Produce an array of LZSS matches for the caller to process. It's safe to overwrite the LZSS graph to do this. */
			matches = (ClownLZSS_Match*)node_meta_array;
			total_matches = 0;

			i = 0;
			while (node_meta_array[i].u.next_node_index != DUMMY)
			{
				const size_t next_index = node_meta_array[i].u.next_node_index;
				const size_t offset = node_meta_array[next_index].match_offset;

				matches[total_matches].source = offset;
				matches[total_matches].destination = i;
				matches[total_matches].length = next_index - i;

				++total_matches;

				i = next_index;
			}

			*_matches = matches;
			*_total_matches = total_matches;
			success = 1;
		}
	}

	return success;
}
