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

#ifndef CLOWNLZSS_DECOMPRESSORS_ENIGMA_H
#define CLOWNLZSS_DECOMPRESSORS_ENIGMA_H

#include <utility>

#include "../bitfield.h"
#include "common.h"

namespace ClownLZSS
{
	namespace Internal
	{
		namespace Enigma
		{
			template<typename T>
			using DecompressorOutput = DecompressorOuputBasic<T>;

			template<typename T>
			using BitField = BitField::Reader<1, BitField::ReadWhen::BeforePop, BitField::PopWhere::High, BitField::Endian::Big, T>;

			template<typename T1, typename T2>
			void Decompress(DecompressorInput<T1> &input, DecompressorOutput<T2> &output)
			{
				const unsigned int total_inline_copy_bits = input.Read();
				const unsigned int render_flags_mask = input.Read();

				unsigned int incremental_copy_word = input.ReadBE16();
				const unsigned int literal_copy_word = input.ReadBE16();

				BitField<decltype(input)> input_bits(input);

				for (;;)
				{
					const auto GetInlineValue = [&]()
					{
						unsigned int render_flags = 0;

						// The Enigma decompressor in the Sonic games only does 5 bits.
						for (unsigned int i = 0; i < 5; ++i)
						{
							const unsigned int bit_index = 5 - i - 1;

							render_flags <<= 1;

							if ((render_flags_mask & 1 << bit_index) != 0)
								render_flags |= input_bits.Pop();
						}

						render_flags <<= 3 + 8;

						return render_flags | input_bits.Pop(total_inline_copy_bits);
					};

					const unsigned int action = input_bits.Pop() ? 2 + input_bits.Pop(2) : input_bits.Pop();

					const unsigned int count = input_bits.Pop(4) + 1;

					if (count == 0x10 && action == 5)
						break;

					switch (action)
					{
						case 0:
							for (unsigned int i = 0; i < count; ++i)
							{
								output.WriteBE16(incremental_copy_word);
								++incremental_copy_word;
							}

							break;

						case 1:
							for (unsigned int i = 0; i < count; ++i)
								output.WriteBE16(literal_copy_word);

							break;

						case 2:
						{
							const unsigned int inline_value = GetInlineValue();

							for (unsigned int i = 0; i < count; ++i)
								output.WriteBE16(inline_value);

							break;
						}

						case 3:
						{
							unsigned int inline_value = GetInlineValue();

							for (unsigned int i = 0; i < count; ++i)
							{
								output.WriteBE16(inline_value);
								++inline_value;
							}

							break;
						}

						case 4:
						{
							unsigned int inline_value = GetInlineValue();

							for (unsigned int i = 0; i < count; ++i)
							{
								output.WriteBE16(inline_value);
								--inline_value;
							}

							break;
						}

						case 5:
							for (unsigned int i = 0; i < count; ++i)
								output.WriteBE16(GetInlineValue());

							break;
					}
				}
			}
		}
	}

	template<typename T1, typename T2>
	void EnigmaDecompress(T1 &&input, T2 &&output)
	{
		using namespace Internal;

		DecompressorInput input_wrapped(std::forward<T1>(input));
		Enigma::DecompressorOutput<T2> output_wrapped(std::forward<T2>(output));
		Enigma::Decompress(input_wrapped, output_wrapped);
	}

	template<typename T1, typename T2>
	void ModuledEnigmaDecompress(T1 &&input, T2 &&output)
	{
		using namespace Internal;

		DecompressorInput input_wrapped(std::forward<T1>(input));
		Enigma::DecompressorOutput<T2> output_wrapped(std::forward<T2>(output));
		ModuledDecompressionWrapper<2, Endian::Big>(input_wrapped, output_wrapped, Enigma::Decompress, 2);
	}
}

#endif // CLOWNLZSS_DECOMPRESSORS_ENIGMA_H
