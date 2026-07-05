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

#ifdef __cplusplus
 // MSVC does not adhere to the C++ standard.
 #ifdef _MSVC_LANG
  #define CLOWNLZSS_CPLUSPLUS _MSVC_LANG
 #else
  #define CLOWNLZSS_CPLUSPLUS __cplusplus
 #endif
#endif

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

#ifdef CLOWNLZSS_CPLUSPLUS
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

#ifdef CLOWNLZSS_CPLUSPLUS
}
#endif

#if defined(CLOWNLZSS_CPLUSPLUS) && CLOWNLZSS_CPLUSPLUS >= 201103L
#include <memory>

namespace ClownLZSS
{
	class Matches
	{
	private:
		struct MatchDeleter
		{
			void operator()(ClownLZSS_Match* const a)
			{
				free(a);
			}
		};

		static constexpr size_t invalid_total_matches = static_cast<size_t>(-1);

		const std::unique_ptr<ClownLZSS_Match[], MatchDeleter> pointer;
		const size_t total_matches;

	public:
		Matches() noexcept
			: pointer(nullptr)
			, total_matches(invalid_total_matches)
		{}

		Matches(ClownLZSS_Match* const matches, const size_t total_matches) noexcept
			: pointer(matches)
			, total_matches(total_matches)
		{}

		auto size() const noexcept {return total_matches;}

		auto begin() noexcept {return pointer.get();}
		auto end() noexcept {return pointer.get() + size();}

		auto begin() const noexcept {return pointer.get();}
		auto end() const noexcept {return pointer.get() + size();}

		auto cbegin() const noexcept {return pointer.get();}
		auto cend() const noexcept {return pointer.get() + size();}

		operator bool() const noexcept {return size() != invalid_total_matches;}
	};

	inline Matches FindOptimalMatches(
		int filler_value,
		size_t maximum_match_length,
		size_t maximum_match_distance,
		void (*extra_matches_callback)(const unsigned char *data, size_t total_values, size_t offset, ClownLZSS_GraphEdge *node_meta_array, void *user),
		size_t literal_cost,
		size_t (*match_cost_callback)(size_t distance, size_t length, void *user),
		const unsigned char *data,
		size_t bytes_per_value,
		size_t total_values,
		const void *user
	)
	{
		ClownLZSS_Match *matches;
		size_t total_matches;

		if (!ClownLZSS_FindOptimalMatches(filler_value, maximum_match_length, maximum_match_distance, extra_matches_callback, literal_cost, match_cost_callback, data, bytes_per_value, total_values, &matches, &total_matches, user))
			return {};

		return {matches, total_matches};
	}
}
#endif

#endif /* CLOWNLZSS_H */
