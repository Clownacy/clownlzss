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

#ifndef CLOWNLZSS_H
#define CLOWNLZSS_H

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
	size_t match_offset;
} ClownLZSS_GraphEdge;

typedef struct ClownLZSS_Match
{
	size_t source;
	size_t destination;
	size_t length;
} ClownLZSS_Match;

#define CLOWNLZSS_MATCH_IS_LITERAL(match) ((match)->source == (match)->destination + 1)

#ifdef __cplusplus
extern "C" {
#endif

int ClownLZSS_FindOptimalMatches(
	int filler_value,
	size_t maximum_match_length,
	size_t maximum_match_distance,
	void (*extra_matches_callback)(const unsigned char *data, size_t total_values, size_t offset, ClownLZSS_GraphEdge *node_meta_array, void *user),
	size_t literal_cost,
	size_t (*match_cost_callback)(size_t distance, size_t length, void *user),
	const unsigned char *data,
	size_t bytes_per_value,
	size_t total_values,
	ClownLZSS_Match **matches,
	size_t *total_matches,
	const void *user
);

#ifdef __cplusplus
}
#endif

#if defined(__cplusplus) && __cplusplus >= 201103L
#include <memory>

namespace ClownLZSS
{
	namespace Internal
	{
		struct MatchDeleter
		{
			void operator()(ClownLZSS_Match* const a)
			{
				free(a);
			}
		};
	}

	using Matches = std::unique_ptr<ClownLZSS_Match[], Internal::MatchDeleter>;

	inline bool FindOptimalMatches(
		int filler_value,
		size_t maximum_match_length,
		size_t maximum_match_distance,
		void (*extra_matches_callback)(const unsigned char *data, size_t total_values, size_t offset, ClownLZSS_GraphEdge *node_meta_array, void *user),
		size_t literal_cost,
		size_t (*match_cost_callback)(size_t distance, size_t length, void *user),
		const unsigned char *data,
		size_t bytes_per_value,
		size_t total_values,
		Matches *matches,
		size_t *total_matches,
		const void *user
	)
	{
		ClownLZSS_Match *matches_pointer;
		const bool success = ClownLZSS_FindOptimalMatches(filler_value, maximum_match_length, maximum_match_distance, extra_matches_callback, literal_cost, match_cost_callback, data, bytes_per_value, total_values, &matches_pointer, total_matches, user);

		*matches = Matches(matches_pointer);

		return success;
	}
}
#endif

#endif /* CLOWNLZSS_H */
