/*
Copyright (c) 2018-2022 Clownacy

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

#ifndef CLOWNLZSS_COMPRESSORS_SAXMAN_H
#define CLOWNLZSS_COMPRESSORS_SAXMAN_H

#include <algorithm>

#include "../clownlzss.h"

#include "common.h"

namespace ClownLZSS
{
	namespace Internal
	{
		namespace Saxman
		{
			inline constexpr unsigned int TOTAL_DESCRIPTOR_BITS = 8;

			inline std::size_t GetMatchCost([[maybe_unused]] const std::size_t distance, [[maybe_unused]] const std::size_t length, [[maybe_unused]] void* const user)
			{
				if (length >= 3)
					return 1 + 16;	// Descriptor bit, offset/length bits.
				else
					return 0;
			}

			inline void FindExtraMatches(const unsigned char* const data, const std::size_t total_values, const std::size_t offset, ClownLZSS_GraphEdge* const node_meta_array, [[maybe_unused]] void* const user)
			{
				if (offset < 0x1000)
				{
					std::size_t i;

					const std::size_t max_read_ahead = std::min<std::size_t>(0x12, total_values - offset);

					for (i = 0; i < max_read_ahead && data[offset + i] == 0; ++i)
					{
						const unsigned int cost = GetMatchCost(0, i + 1, user);

						if (cost && node_meta_array[offset + i + 1].u.cost > node_meta_array[offset].u.cost + cost)
						{
							node_meta_array[offset + i + 1].u.cost = node_meta_array[offset].u.cost + cost;
							node_meta_array[offset + i + 1].previous_node_index = offset;
							node_meta_array[offset + i + 1].match_offset = 0xFFF;
						}
					}
				}
			}

			template<typename T>
			inline bool Compress(const unsigned char* const data, const std::size_t data_size, T &&output)
			{
				ClownLZSS_Match *matches;
				std::size_t total_matches;

				// Produce a series of LZSS compression matches.
				if (!ClownLZSS_Compress(0x12, 0x1000, FindExtraMatches, 1 + 8, GetMatchCost, data, 1, data_size, &matches, &total_matches, nullptr))
					return false;

				/* Set up the state. */
				typename std::remove_cvref_t<T>::pos_type descriptor_position;
				unsigned int descriptor = 0;
				unsigned int descriptor_bits_remaining;

				const auto BeginDescriptorField = [&]()
				{
					descriptor_bits_remaining = TOTAL_DESCRIPTOR_BITS;

					// Log the placement of the descriptor field.
					descriptor_position = output.Tell();

					// Insert a placeholder.
					output.Write(0);
				};

				const auto FinishDescriptorField = [&]()
				{
					// Back up current position.
					const auto current_position = output.Tell();

					// Go back to the descriptor field.
					output.Seek(descriptor_position);

					// Write the complete descriptor field.
					output.Write(descriptor & 0xFF);

					// Seek back to where we were before.
					output.Seek(current_position);
				};

				const auto PutDescriptorBit = [&](const bool bit)
				{
					if (descriptor_bits_remaining == 0)
					{
						FinishDescriptorField();
						BeginDescriptorField();
					}

					--descriptor_bits_remaining;

					descriptor >>= 1;

					if (bit)
						descriptor |= 1 << (TOTAL_DESCRIPTOR_BITS - 1);
				};

				// Begin first descriptor field.
				BeginDescriptorField();

				// Produce Saxman-formatted data.
				for (ClownLZSS_Match *match = matches; match != &matches[total_matches]; ++match)
				{
					if (CLOWNLZSS_MATCH_IS_LITERAL(match))
					{
						PutDescriptorBit(1);
						output.Write(data[match->destination]);
					}
					else
					{
						const std::size_t offset = match->source - 0x12;
						const std::size_t length = match->length;

						PutDescriptorBit(0);
						output.Write(offset & 0xFF);
						output.Write(((offset & 0xF00) >> 4) | (length - 3));
					}
				}

				// We don't need the matches anymore.
				free(matches);

				// The descriptor field may be incomplete, so move the bits into their proper place.
				descriptor >>= descriptor_bits_remaining;

				// Finish last descriptor field.
				FinishDescriptorField();

				return true;
			}

			template<typename T>
			inline bool CompressWithHeader(const unsigned char* const data, const std::size_t data_size, T &&output)
			{
				// Track the location of the header...
				const auto header_position = output.Tell();

				// ...and insert a placeholder there.
				output.Write(0);
				output.Write(0);

				if (!Compress(data, data_size, output))
					return false;

				// Grab the current position for later.
				const auto end_position = output.Tell();

				// Rewind to the header...
				output.Seek(header_position);

				// ...and complete it.
				output.Write(((end_position - header_position - 2) >> (8 * 0)) & 0xFF);
				output.Write(((end_position - header_position - 2) >> (8 * 1)) & 0xFF);

				// Seek back to the end of the file just as the caller might expect us to do.
				output.Seek(end_position);

				return true;
			}

			template<typename T>
			inline bool CompressWithoutHeader(const unsigned char* const data, const std::size_t data_size, T &&output)
			{
				return Compress(data, data_size, output);
			}
		}
	}

	template<typename T>
	bool SaxmanCompressWithoutHeader(const unsigned char* const data, const std::size_t data_size, T &&output)
	{
		using namespace Internal;

		return Saxman::CompressWithoutHeader(data, data_size, CompressorOutput(output));
	}

	template<typename T>
	bool SaxmanCompressWithHeader(const unsigned char* const data, const std::size_t data_size, T &&output)
	{
		using namespace Internal;

		return Saxman::CompressWithHeader(data, data_size, CompressorOutput(output));
	}

	template<typename T>
	bool ModuledSaxmanCompressWithoutHeader(const unsigned char* const data, const std::size_t data_size, T &&output, const size_t module_size, const size_t module_alignment)
	{
		return Internal::ModuledCompressionWrapper(data, data_size, CompressorOutput(output), SaxmanCompressWithoutHeader, module_size, module_alignment);
	}

	template<typename T>
	bool ModuledSaxmanCompressWithHeader(const unsigned char* const data, const std::size_t data_size, T &&output, const size_t module_size, const size_t module_alignment)
	{
		return Internal::ModuledCompressionWrapper(data, data_size, CompressorOutput(output), SaxmanCompressWithHeader, module_size, module_alignment);
	}
}

#endif // CLOWNLZSS_COMPRESSORS_SAXMAN_H
