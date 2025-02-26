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

#ifndef CLOWNLZSS_COMPRESSORS_COMPER_H
#define CLOWNLZSS_COMPRESSORS_COMPER_H

#include <utility>

#include "../bitfield.h"
#include "clownlzss.h"
#include "common.h"

namespace ClownLZSS
{
	namespace Internal
	{
		namespace Comper
		{
			template<typename T>
			using DescriptorFieldWriter = BitField::DescriptorFieldWriter<2, BitField::WriteWhen::BeforePush, BitField::PushWhere::Low, BitField::Endian::Big, T>;

			inline std::size_t GetMatchCost([[maybe_unused]] const std::size_t distance, [[maybe_unused]] const std::size_t length, [[maybe_unused]] void* const user)
			{
				return 1 + 16;	// Descriptor bit, offset/length bytes.
			}

			template<typename T>
			bool Compress(const unsigned char* const data, const std::size_t data_size, CompressorOutput<T> &output)
			{
				constexpr unsigned int bytes_per_value = 2;

				// Cannot compress data that is an odd number of bytes long.
				if (data_size % bytes_per_value != 0)
					return false;

				// Produce a series of LZSS compression matches.
				ClownLZSS::Matches matches;
				std::size_t total_matches;
				if (!ClownLZSS::FindOptimalMatches(-1, 0x100, 0x100, nullptr, 1 + 16, GetMatchCost, data, bytes_per_value, data_size / bytes_per_value, &matches, &total_matches, nullptr))
					return false;

				DescriptorFieldWriter<decltype(output)> descriptor_bits(output);

				// Produce Comper-formatted data.
				for (ClownLZSS_Match *match = &matches[0]; match != &matches[total_matches]; ++match)
				{
					if (CLOWNLZSS_MATCH_IS_LITERAL(match))
					{
						descriptor_bits.Push(0);
						output.Write(data[match->destination * 2 + 0]);
						output.Write(data[match->destination * 2 + 1]);
					}
					else
					{
						const std::size_t distance = match->destination - match->source;
						const std::size_t length = match->length;

						descriptor_bits.Push(1);
						output.Write(-distance & 0xFF);
						output.Write(length - 1);
					}
				}

				// Add the terminator match.
				descriptor_bits.Push(1);
				output.Write(0);
				output.Write(0);

				return true;
			}
		}
	}

	template<typename T>
	bool ComperCompress(const unsigned char* const data, const std::size_t data_size, T &&output)
	{
		using namespace Internal;

		CompressorOutput output_wrapped(std::forward<T>(output));
		return Comper::Compress(data, data_size, output_wrapped);
	}

	template<typename T>
	bool ModuledComperCompress(const unsigned char* const data, const std::size_t data_size, T &&output, const std::size_t module_size)
	{
		using namespace Internal;

		CompressorOutput output_wrapped(std::forward<T>(output));
		return ModuledCompressionWrapper<2, Endian::Big>(data, data_size, output_wrapped, Comper::Compress, module_size, 2);
	}
}

#endif // CLOWNLZSS_COMPRESSORS_COMPER_H
