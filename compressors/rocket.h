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

#ifndef CLOWNLZSS_COMPRESSORS_ROCKET_H
#define CLOWNLZSS_COMPRESSORS_ROCKET_H

#include <utility>

#include "../bitfield.h"
#include "clownlzss.h"
#include "common.h"

namespace ClownLZSS
{
	namespace Internal
	{
		namespace Rocket
		{
			template<typename T>
			using DescriptorFieldWriter = BitField::DescriptorFieldWriter<1, BitField::WriteWhen::BeforePush, BitField::PushWhere::High, BitField::Endian::Big, T>;

			inline std::size_t GetMatchCost([[maybe_unused]] const std::size_t distance, [[maybe_unused]] const std::size_t length, [[maybe_unused]] void* const user)
			{
				return 1 + 16;	// Descriptor bit, offset/length bytes.
			}

			template<typename T>
			bool Compress(const unsigned char* const data, const std::size_t data_size, CompressorOutput<T> &output)
			{
				// Produce a series of LZSS compression matches.
				ClownLZSS::Matches matches;
				std::size_t total_matches;
				if (!ClownLZSS::FindOptimalMatches(0x20, 0x40, 0x400, nullptr, 1 + 8, GetMatchCost, data, 1, data_size, &matches, &total_matches, nullptr))
					return false;

				// Write the first part of the header.
				output.WriteBE16(data_size);

				// Track the location of the second part of the header...
				const auto header_position = output.Tell();

				// ...and insert a placeholder there.
				output.WriteBE16(0);

				DescriptorFieldWriter<decltype(output)> descriptor_bits(output);

				// Produce Rocket-formatted data.
				for (ClownLZSS_Match *match = &matches[0]; match != &matches[total_matches]; ++match)
				{
					if (CLOWNLZSS_MATCH_IS_LITERAL(match))
					{
						descriptor_bits.Push(1);
						output.Write(data[match->destination]);
					}
					else
					{
						const std::size_t offset = (match->source - 0x40) % 0x400;
						const std::size_t length = match->length;

						descriptor_bits.Push(0);
						output.Write(((offset >> 8) & 3) | ((length - 1) << 2));
						output.Write(offset & 0xFF);
					}
				}

				// Grab the current position for later.
				const auto end_position = output.Tell();

				// Rewind to the header...
				output.Seek(header_position);

				// ...and complete it.
				output.WriteBE16(end_position - header_position - 2);

				// Seek back to the end of the file just as the caller might expect us to do.
				output.Seek(end_position);

				return true;
			}
		}
	}

	template<typename T>
	bool RocketCompress(const unsigned char* const data, const std::size_t data_size, T &&output)
	{
		using namespace Internal;

		CompressorOutput output_wrapped(std::forward<T>(output));
		return Rocket::Compress(data, data_size, output_wrapped);
	}

	template<typename T>
	bool ModuledRocketCompress(const unsigned char* const data, const std::size_t data_size, T &&output, const std::size_t module_size)
	{
		using namespace Internal;

		CompressorOutput output_wrapped(std::forward<T>(output));
		return ModuledCompressionWrapper<2, Endian::Big>(data, data_size, output_wrapped, Rocket::Compress, module_size, 2);
	}
}

#endif // CLOWNLZSS_COMPRESSORS_ROCKET_H
