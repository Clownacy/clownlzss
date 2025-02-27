/*
Copyright (c) 2025 Thomas Mathys

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

#ifndef CLOWNLZSS_COMPRESSORS_GBA_H
#define CLOWNLZSS_COMPRESSORS_GBA_H

#include <cassert>
#include <cstddef>
#include <ranges>

#include "clownlzss.h"
#include "common.h"

namespace ClownLZSS
{
	namespace Internal
	{
		namespace Gba
		{
			namespace Compressor
			{
				inline constexpr auto bios_compression_type = 0x10;
				inline constexpr auto filler_value = -1;
				inline constexpr auto minimum_match_length = 3;
				inline constexpr auto maximum_match_length = 18;
				inline constexpr auto minimum_match_distance = 1;
				inline constexpr auto minimum_match_distance_vram_safe = 2;
				inline constexpr auto maximum_match_distance = 0x1000;
				inline constexpr auto literal_cost = 1 + 8;
				inline constexpr auto match_cost = 1 + 16;
				inline constexpr auto bytes_per_value = 1;
				inline constexpr auto maximum_encoded_length = 0xfu;
				inline constexpr auto maximum_encoded_offset = 0xfffu;
				inline constexpr auto module_header_size = 4;
				inline constexpr auto module_alignment = 4;
			}

			template<typename T>
			using DescriptorFieldWriter = BitField::DescriptorFieldWriter<1, BitField::WriteWhen::BeforePush, BitField::PushWhere::Low, BitField::Endian::Big, T>;

			bool IsInRange(std::unsigned_integral auto x, std::unsigned_integral auto min, std::unsigned_integral auto max)
			{
				return (min <= x) && (x <= max);
			}

			inline std::size_t GetMatchCost(const std::size_t, const std::size_t length, void* const)
			{
				using namespace Compressor;

				if (length < minimum_match_length)
					return 0;

				return match_cost;
			}

			inline std::size_t GetMatchCostVramSafe(const std::size_t distance, const std::size_t length, void* const)
			{
				using namespace Compressor;

				if ((length < minimum_match_length) || (distance < minimum_match_distance_vram_safe))
					return 0;

				return match_cost;
			}

			template<typename T>
			auto ReserveSpaceForHeader(CompressorOutput<T> &output)
			{
				const auto header_position = output.Tell();
				output.WriteLE16(0);
				output.WriteLE16(0);
				return header_position;
			}

			template<typename T>
			void WriteHeader(const auto header_position, const std::size_t data_size, CompressorOutput<T> &output)
			{
				using namespace Compressor;

				const auto old_position = output.Tell();
				output.Seek(header_position);
				output.Write(bios_compression_type);
				output.Write(data_size & 255);
				output.Write((data_size >> 8) & 255);
				output.Write((data_size >> 16) & 255);
				output.Seek(old_position);
			}

			template<typename T>
			void WritePaddingBytes(const auto header_position, CompressorOutput<T> &output)
			{
				// The GBA BIOS requires both the size of compressed data and its position in memory
				// to be a multiple of 4 bytes, including the header.
				// We do align the total number of bytes written here, but we do not align the absolute position.
				// The latter would be the responsibility of whatever puts the compressed data into the GBA's memory.
				while (output.Distance(header_position) % 4 != 0)
				{
					output.Write(0);
				}
			}

			inline unsigned int EncodeMatch(const ClownLZSS_Match& match)
			{
				using namespace Compressor;

				const std::size_t length = match.length - minimum_match_length;
				const std::size_t offset = match.destination - match.source - 1;

				assert(IsInRange(length, 0u, maximum_encoded_length));
				assert(IsInRange(offset, 0u, maximum_encoded_offset));

				return ((offset & 255) << 8) | ((length) << 4) | (offset >> 8);
			}

			template<typename T>
			void EncodeMatches(const unsigned char* const data, const Matches& matches, std::size_t total_matches, CompressorOutput<T> &output)
			{
				DescriptorFieldWriter<decltype(output)> descriptor_bits(output);
				for (const auto& match : std::ranges::subrange(&matches[0], &matches[total_matches]))
				{
					if (CLOWNLZSS_MATCH_IS_LITERAL(&match))
					{
						descriptor_bits.Push(0);
						output.Write(data[match.destination]);
					}
					else
					{
						descriptor_bits.Push(1);
						output.WriteLE16(EncodeMatch(match));
					}
				}
			}

			template<typename T>
			bool Compress(const unsigned char* const data, const std::size_t data_size, auto match_cost_callback, CompressorOutput<T> &output)
			{
				using namespace Compressor;

				// Produce a series of LZSS compression matches.
				ClownLZSS::Matches matches;
				std::size_t total_matches;
				if (!ClownLZSS::FindOptimalMatches(filler_value, maximum_match_length, maximum_match_distance, nullptr, literal_cost, match_cost_callback, data, bytes_per_value, data_size / bytes_per_value, &matches, &total_matches, nullptr))
					return false;

				const auto header_position = ReserveSpaceForHeader(output);
				EncodeMatches(data, matches, total_matches, output);
				WriteHeader(header_position, data_size, output);
				WritePaddingBytes(header_position, output);

				return true;
			}

			template<typename T>
			bool Compress(const unsigned char* const data, const std::size_t data_size, CompressorOutput<T> &output)
			{
				return Compress(data, data_size, GetMatchCost, output);
			}

			template<typename T>
			bool CompressVramSafe(const unsigned char* const data, const std::size_t data_size, CompressorOutput<T> &output)
			{
				return Compress(data, data_size, GetMatchCostVramSafe, output);
			}
		}
	}

	template<typename T>
	bool GbaCompress(const unsigned char* const data, const std::size_t data_size, T &&output)
	{
		using namespace Internal;

		CompressorOutput output_wrapped(std::forward<T>(output));
		return Gba::Compress(data, data_size, output_wrapped);
	}

	template<typename T>
	bool GbaVramSafeCompress(const unsigned char* const data, const std::size_t data_size, T &&output)
	{
		using namespace Internal;

		CompressorOutput output_wrapped(std::forward<T>(output));
		return Gba::CompressVramSafe(data, data_size, output_wrapped);
	}

	template<typename T>
	bool ModuledGbaCompress(const unsigned char* const data, const std::size_t data_size, T &&output, const std::size_t module_size)
	{
		using namespace Internal;

		CompressorOutput output_wrapped(std::forward<T>(output));
		return ModuledCompressionWrapper<Gba::Compressor::module_header_size, Endian::Little>(data, data_size, output_wrapped, Gba::Compress, module_size, Gba::Compressor::module_alignment);
	}

	template<typename T>
	bool ModuledGbaVramSafeCompress(const unsigned char* const data, const std::size_t data_size, T &&output, const std::size_t module_size)
	{
		using namespace Internal;

		CompressorOutput output_wrapped(std::forward<T>(output));
		return ModuledCompressionWrapper<Gba::Compressor::module_header_size, Endian::Little>(data, data_size, output_wrapped, Gba::CompressVramSafe, module_size, Gba::Compressor::module_alignment);
	}
}

#endif
