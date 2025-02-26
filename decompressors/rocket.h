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

#ifndef CLOWNLZSS_DECOMPRESSORS_ROCKET_H
#define CLOWNLZSS_DECOMPRESSORS_ROCKET_H

#include <utility>

#include "../bitfield.h"
#include "common.h"

namespace ClownLZSS
{
	namespace Internal
	{
		namespace Rocket
		{
			template<typename T>
			using DecompressorOutput = DecompressorOutput<T, 0x400, 0x40, 0x20>;

			template<typename T>
			using BitField = BitField::Reader<1, BitField::ReadWhen::BeforePop, BitField::PopWhere::Low, BitField::Endian::Big, T>;

			template<typename T1, typename T2>
			void Decompress(DecompressorInput<T1> &input, DecompressorOutput<T2> &output)
			{
				const unsigned int uncompressed_size = input.ReadBE16();
				const unsigned int compressed_size = input.ReadBE16();

				const auto input_start_position = input.Tell();
				const auto output_start_position = output.Tell();

				BitField<decltype(input)> descriptor_bits(input);

				while (input.Distance(input_start_position) < compressed_size)
				{
					const unsigned int output_position = output.Distance(output_start_position);

					if (output_position >= uncompressed_size)
						break;

					if (descriptor_bits.Pop())
					{
						// Uncompressed.
						output.Write(input.Read());
					}
					else
					{
						// Dictionary match.
						const unsigned int word = input.ReadBE16();
						const unsigned int dictionary_index = (word + 0x40) % 0x400;
						const unsigned int count = (word >> 10) + 1;
						const unsigned int distance = ((0x400 + output_position - dictionary_index - 1) % 0x400) + 1;

						output.Copy(distance, count);
					}
				}
			}
		}
	}

	template<typename T1, typename T2>
	void RocketDecompress(T1 &&input, T2 &&output)
	{
		using namespace Internal;

		DecompressorInput input_wrapped(std::forward<T1>(input));
		Rocket::DecompressorOutput<T2> output_wrapped(std::forward<T2>(output));
		Rocket::Decompress(input_wrapped, output_wrapped);
	}

	template<typename T1, typename T2>
	void ModuledRocketDecompress(T1 &&input, T2 &&output)
	{
		using namespace Internal;

		DecompressorInput input_wrapped(std::forward<T1>(input));
		Rocket::DecompressorOutput<T2> output_wrapped(std::forward<T2>(output));
		ModuledDecompressionWrapper<2, Endian::Big>(input_wrapped, output_wrapped, Rocket::Decompress, 2);
	}
}

#endif // CLOWNLZSS_DECOMPRESSORS_ROCKET_H
