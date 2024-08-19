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

#ifndef CLOWNLZSS_DECOMPRESSORS_CHAMELEON_H
#define CLOWNLZSS_DECOMPRESSORS_CHAMELEON_H

#include <utility>

#include "bitfield.h"
#include "common.h"

namespace ClownLZSS
{
	namespace Internal
	{
		namespace Chameleon
		{
			template<typename T>
			using DecompressorOutput = DecompressorOutput<T, 0x7FF, 0xFF>;

			template<typename T>
			using BitField = BitField<1, ReadWhen::BeforePop, PopWhere::High, Endian::Big, T>;

			template<typename T1, typename T2, typename T3>
			void Decompress(DecompressorInput<T1> &input, DecompressorOutput<T2> &output, DecompressorInputSeparate<T3> &descriptor_input)
			{
				input += input.ReadBE16();
				descriptor_input += 2;

				BitField descriptor_bits(descriptor_input);

				for (;;)
				{
					if (descriptor_bits.Pop())
					{
						output.Write(input.Read());
					}
					else
					{
						unsigned int distance = input.Read();
						unsigned int count;

						if (!descriptor_bits.Pop())
						{
							count = 2 + descriptor_bits.Pop();
						}
						else
						{
							if (descriptor_bits.Pop())
								distance += 1 << 10;
							if (descriptor_bits.Pop())
								distance += 1 << 9;
							if (descriptor_bits.Pop())
								distance += 1 << 8;

							if (!descriptor_bits.Pop())
							{
								if (!descriptor_bits.Pop())
									count = 3;
								else
									count = 4;
							}
							else
							{
								if (!descriptor_bits.Pop())
								{
									count = 5;
								}
								else
								{
									count = input.Read();

									if (count < 6)
										break;
								}
							}
						}

						output.Copy(distance, count);
					}
				}
			}
		}
	}

	template<typename T1, typename T2>
	void ChameleonDecompress(T1 &&input, T2 &&output)
	{
		using namespace Internal;

		DecompressorInput input_wrapped(std::forward<T1>(input));
		Chameleon::DecompressorOutput output_wrapped(std::forward<T2>(output));
		DecompressorInputSeparate input_separate(std::forward<T1>(input));
		Chameleon::Decompress(input_wrapped, output_wrapped, input_separate);
	}
}

#endif // CLOWNLZSS_DECOMPRESSORS_CHAMELEON_H
