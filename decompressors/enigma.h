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

#include "bitfield.h"
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
			using BitField = BitField<1, ReadWhen::BeforePop, PopWhere::High, Endian::Big, T>;

			template<typename T1, typename T2>
			void Decompress(DecompressorInput<T1> &input, DecompressorOutput<T2> &output)
			{
				const unsigned int total_inline_copy_bits = input.Read();
				const unsigned int render_flags_mask = input.Read();

				unsigned int incremental_copy_word = input.ReadBE16();
				const unsigned int literal_copy_word = input.ReadBE16();

				BitField input_bits(input);

				for (;;)
				{
					const auto GetCountValue = [&]()
					{
						return input_bits.Pop(4) + 1;	
					};

					if (!input_bits.Pop())
					{
						const bool bit = input_bits.Pop();

						const unsigned int count = GetCountValue();

						if (!bit)
						{
							for (unsigned int i = 0; i < count; ++i)
								output.WriteBE16(incremental_copy_word);

							++incremental_copy_word;
						}
						else
						{
							for (unsigned int i = 0; i < count; ++i)
								output.WriteBE16(literal_copy_word);
						}
					}
					else
					{
						const auto GetInlineValue = [&]()
						{
							unsigned int render_flags = 0;

							// TODO: Does the Enigma decompressor in the Sonic games do 5 bits or all 8 bits?
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

						const bool bit1 = input_bits.Pop();
						const bool bit2 = input_bits.Pop();

						const unsigned int count = GetCountValue();

						if (!bit1)
						{
							if (!bit2)
							{
								const unsigned int inline_value = GetInlineValue();

								for (unsigned int i = 0; i < count; ++i)
									output.WriteBE16(inline_value);
							}
							else
							{
								unsigned int inline_value = GetInlineValue();

								for (unsigned int i = 0; i < count; ++i)
								{
									output.WriteBE16(inline_value);
									++inline_value;
								}
							}
						}
						else
						{
							if (!bit2)
							{
								unsigned int inline_value = GetInlineValue();

								for (unsigned int i = 0; i < count; ++i)
								{
									output.WriteBE16(inline_value);
									--inline_value;
								}
							}
							else
							{
								if (count == 0x10)
									break;

								for (unsigned int i = 0; i < count; ++i)
									output.WriteBE16(GetInlineValue());
							}
						}
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
		Enigma::DecompressorOutput output_wrapped(std::forward<T2>(output));
		Enigma::Decompress(input_wrapped, output_wrapped);
	}

	template<typename T1, typename T2>
	void ModuledEnigmaDecompress(T1 &&input, T2 &&output)
	{
		using namespace Internal;

		DecompressorInput input_wrapped(std::forward<T1>(input));
		Enigma::DecompressorOutput output_wrapped(std::forward<T2>(output));
		ModuledDecompressionWrapper(input_wrapped, output_wrapped, Enigma::Decompress, 2);
	}
}

#endif // CLOWNLZSS_DECOMPRESSORS_ENIGMA_H
