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

#ifndef CLOWNLZSS_DECOMPRESSORS_COMPER_H
#define CLOWNLZSS_DECOMPRESSORS_COMPER_H

#include <utility>

#include "../bitfield.h"
#include "common.h"

namespace ClownLZSS
{
	namespace Internal
	{
		namespace Comper
		{
			constexpr unsigned int RawDistanceToDistance(const unsigned int raw_distance)
			{
				return (0x100 - raw_distance) * 2;
			}

			constexpr unsigned int RawCountToCount(const unsigned int raw_count)
			{
				return (raw_count + 1) * 2;
			}

			template<typename T>
			using DecompressorOutput = DecompressorOutput<T, RawDistanceToDistance(0), RawCountToCount(0xFF)>;

			template<typename T>
			using BitField = BitField::Reader<2, BitField::ReadWhen::BeforePop, BitField::PopWhere::High, BitField::Endian::Big, T>;

			template<typename T1, typename T2>
			void Decompress(DecompressorInput<T1> &input, DecompressorOutput<T2> &output)
			{
				BitField<decltype(input)> descriptor_bits(input);

				for (;;)
				{
					if (!descriptor_bits.Pop())
					{
						// Uncompressed.
						output.Write(input.Read());
						output.Write(input.Read());
					}
					else
					{
						// Dictionary match.
						const unsigned int raw_distance = input.Read();
						const unsigned int distance = RawDistanceToDistance(raw_distance);
						const unsigned int raw_count = input.Read();
						const unsigned int count = RawCountToCount(raw_count);

						if (raw_count == 0)
							break;

						output.Copy(distance, count);
					}
				}
			}
		}
	}

	template<typename T1, typename T2>
	void ComperDecompress(T1 &&input, T2 &&output)
	{
		using namespace Internal;

		DecompressorInput input_wrapped(std::forward<T1>(input));
		Comper::DecompressorOutput<T2> output_wrapped(std::forward<T2>(output));
		Comper::Decompress(input_wrapped, output_wrapped);
	}

	template<typename T1, typename T2>
	void ModuledComperDecompress(T1 &&input, T2 &&output)
	{
		using namespace Internal;

		DecompressorInput input_wrapped(std::forward<T1>(input));
		Comper::DecompressorOutput<T2> output_wrapped(std::forward<T2>(output));
		ModuledDecompressionWrapper<2, Endian::Big>(input_wrapped, output_wrapped, Comper::Decompress, 2);
	}
}

#endif // CLOWNLZSS_DECOMPRESSORS_COMPER_H
