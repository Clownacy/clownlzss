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

#ifndef CLOWNLZSS_DECOMPRESSORS_RAGE_H
#define CLOWNLZSS_DECOMPRESSORS_RAGE_H

#include <utility>

#include "common.h"

namespace ClownLZSS
{
	namespace Internal
	{
		namespace Rage
		{
			template<typename T>
			using DecompressorOutput = DecompressorOutput<T, 0x1FFF, 0x1F>;

			template<typename T1, typename T2>
			void Decompress(DecompressorInput<T1> &input, DecompressorOutput<T2> &output)
			{
				const auto input_start_position = input.Tell();

				const unsigned int compressed_size = input.ReadLE16();

				unsigned int distance = 0; // TODO: What is this initialised to in Streets of Rage's decompressor?

				while (input.Distance(input_start_position) < compressed_size)
				{
					const unsigned int first_byte = input.Read();

					switch (first_byte >> 5)
					{
						case 0:
						case 1:
						{
							unsigned int count;

							if ((first_byte & 0x20) != 0)
								count = ((first_byte << 8) & 0x1F00) | input.Read();
							else
								count = first_byte;

							// TODO: There should be a helper function for this, so that it can be optimised for file IO.
							for (unsigned int i = 0; i < count; ++i)
								output.Write(input.Read());

							break;
						}

						case 2:
						{
							unsigned int count = 4;

							if ((first_byte & 0x10) != 0)
								count += ((first_byte << 8) & 0xF00) | input.Read();
							else
								count += first_byte & 0xF;

							const unsigned char value = input.Read();

							output.Fill(value, count);
							break;
						}

						case 3:
						{
							const unsigned int count = first_byte & 0x1F;

							output.Copy(distance, count);
							break;
						}

						case 4:
						case 5:
						case 6:
						case 7:
						{
							const unsigned int second_byte = input.Read();
							const unsigned int count = ((first_byte >> 5) & 3) + 4;

							distance = ((first_byte << 8) & 0x1F00) | second_byte;
							output.Copy(distance, count);
							break;
						}
					}
				}
			}
		}
	}

	template<typename T1, typename T2>
	void RageDecompress(T1 &&input, T2 &&output)
	{
		using namespace Internal;

		DecompressorInput input_wrapped(std::forward<T1>(input));
		Rage::DecompressorOutput<T2> output_wrapped(std::forward<T2>(output));
		Rage::Decompress(input_wrapped, output_wrapped);
	}

	template<typename T1, typename T2>
	void ModuledRageDecompress(T1 &&input, T2 &&output)
	{
		using namespace Internal;

		DecompressorInput input_wrapped(std::forward<T1>(input));
		Rage::DecompressorOutput<T2> output_wrapped(std::forward<T2>(output));
		ModuledDecompressionWrapper<2, Endian::Big>(input_wrapped, output_wrapped, Rage::Decompress, 2);
	}
}

#endif // CLOWNLZSS_DECOMPRESSORS_RAGE_H
