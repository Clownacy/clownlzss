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

#ifndef CLOWNLZSS_DECOMPRESSORS_GBA_H
#define CLOWNLZSS_DECOMPRESSORS_GBA_H

#include "common.h"

namespace ClownLZSS
{
	namespace Internal
	{
		namespace Gba
		{
			namespace Decompressor
			{
				inline constexpr auto minimum_match_length = 3;
				inline constexpr auto maximum_match_length = 18;
				inline constexpr auto minimum_match_distance = 1;
				inline constexpr auto maximum_match_distance = 0x1000;
				inline constexpr auto module_header_size = 4;
				inline constexpr auto module_alignment = 4;
			}

			template<typename T>
			using DecompressorOutput = DecompressorOutput<T, Decompressor::maximum_match_distance, Decompressor::maximum_match_length>;

			template<typename T>
			using BitField = BitField::Reader<1, BitField::ReadWhen::BeforePop, BitField::PopWhere::High, BitField::Endian::Big, T>;

			template<typename T1, typename T2>
			void Decompress(DecompressorInput<T1> &input, DecompressorOutput<T2> &output, unsigned int uncompressed_size)
			{
				const auto output_start_position = output.Tell();

				BitField<decltype(input)> descriptor_bits(input);

				while (output.Distance(output_start_position) < uncompressed_size)
				{
					if (!descriptor_bits.Pop())
					{
						// Literal
						output.Write(input.Read());
					}
					else
					{
						// Match
						const unsigned int b0 = input.Read();
						const unsigned int b1 = input.Read();
						const unsigned int count = ((b0 >> 4) & 0xf) + Decompressor::minimum_match_length;
						const unsigned int distance = (((b0 & 0xfu) << 8) | b1) + Decompressor::minimum_match_distance;
						output.Copy(distance, count);
					}
				}
			}

			template<typename T>
			unsigned int ReadHeader(DecompressorInput<T> &input)
			{
				input.Read(); // Skip compression type byte
				return input.Read() + (input.Read() << 8) + (input.Read() << 16);
			}

			template<typename T1, typename T2>
			void Decompress(DecompressorInput<T1> &input, DecompressorOutput<T2> &output)
			{
				unsigned int uncompressed_size = ReadHeader(input);
				Decompress(input, output, uncompressed_size);
			}
		}
	}

	template<typename T1, typename T2>
	void GbaDecompress(T1 &&input, T2 &&output)
	{
		using namespace Internal;

		DecompressorInput input_wrapped(std::forward<T1>(input));
		Gba::DecompressorOutput<T2> output_wrapped(std::forward<T2>(output));
		Gba::Decompress(input_wrapped, output_wrapped);
	}

	template<typename T1, typename T2>
	void ModuledGbaDecompress(T1 &&input, T2 &&output)
	{
		using namespace Internal;

		DecompressorInput input_wrapped(std::forward<T1>(input));
		Gba::DecompressorOutput<T2> output_wrapped(std::forward<T2>(output));
		ModuledDecompressionWrapper<Gba::Decompressor::module_header_size, Endian::Little>(input_wrapped, output_wrapped, Gba::Decompress, Gba::Decompressor::module_alignment);
	}
}

#endif
