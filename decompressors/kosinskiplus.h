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

#ifndef CLOWNLZSS_DECOMPRESSORS_KOSINSKIPLUS_H
#define CLOWNLZSS_DECOMPRESSORS_KOSINSKIPLUS_H

#include <utility>

#include "../bitfield.h"
#include "common.h"

namespace ClownLZSS
{
	namespace Internal
	{
		namespace KosinskiPlus
		{
			template<typename T>
			using DecompressorOutput = DecompressorOutput<T, 0x2000, 0x100>;

			template<typename T>
			using BitField = BitField::Reader<1, BitField::ReadWhen::BeforePop, BitField::PopWhere::High, BitField::Endian::Big, T>;

			template<typename T1, typename T2>
			void Decompress(DecompressorInput<T1> &input, DecompressorOutput<T2> &output)
			{
				BitField<decltype(input)> descriptor_bits(input);

				for (;;)
				{
					if (descriptor_bits.Pop())
					{
						output.Write(input.Read());
					}
					else
					{
						unsigned int offset;
						unsigned int count;

						if (descriptor_bits.Pop())
						{
							const unsigned int high_byte = input.Read();
							const unsigned int low_byte = input.Read();

							offset = ((high_byte & 0xF8) << 5) | low_byte;
							offset = 0x2000 - offset;
							count = high_byte & 7;

							if (count != 0)
							{
								count = 10 - count;
							}
							else
							{
								count = input.Read() + 9;

								if (count == 9)
									break;
							}
						}
						else
						{
							offset = 0x100 - input.Read();

							count = 2;

							if (descriptor_bits.Pop())
								count += 2;
							if (descriptor_bits.Pop())
								count += 1;
						}

						output.Copy(offset, count);
					}
				}
			}
		}
	}

	template<typename T1, typename T2>
	void KosinskiPlusDecompress(T1 &&input, T2 &&output)
	{
		using namespace Internal;

		DecompressorInput input_wrapped(std::forward<T1>(input));
		KosinskiPlus::DecompressorOutput<T2> output_wrapped(std::forward<T2>(output));
		KosinskiPlus::Decompress(input_wrapped, output_wrapped);
	}

	template<typename T1, typename T2>
	void ModuledKosinskiPlusDecompress(T1 &&input, T2 &&output)
	{
		using namespace Internal;

		DecompressorInput input_wrapped(std::forward<T1>(input));
		KosinskiPlus::DecompressorOutput<T2> output_wrapped(std::forward<T2>(output));
		ModuledDecompressionWrapper<2, Endian::Big>(input_wrapped, output_wrapped, KosinskiPlus::Decompress, 1);
	}
}

#endif // CLOWNLZSS_DECOMPRESSORS_KOSINSKI_H
