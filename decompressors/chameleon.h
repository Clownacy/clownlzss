#ifndef CLOWNLZSS_DECOMPRESSORS_CHAMELEON_H
#define CLOWNLZSS_DECOMPRESSORS_CHAMELEON_H

#include "common.h"

namespace ClownLZSS
{
	namespace Internal
	{
		namespace Chameleon
		{
			template<typename T>
			using DecompressorOutput = DecompressorOutput<T, 0x7FF, 0xFF>;

			template<typename T1, typename T2, typename T3>
			void Decompress(T1 &&input, T2 &&output, T3 &&descriptor_input)
			{
				BitField<1, ReadWhen::BeforePop, PopWhere::High, Endian::Big, T3> descriptor_bits(descriptor_input);

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

		DecompressorInput wrapped_input(input);
		DecompressorInputSeparate descriptor_input(input);

		const unsigned int descriptor_buffer_size_upper_byte = wrapped_input.Read();
		const unsigned int descriptor_buffer_size_lower_byte = wrapped_input.Read();
		const unsigned int descriptor_buffer_size = descriptor_buffer_size_upper_byte << 8 | descriptor_buffer_size_lower_byte;

		wrapped_input += descriptor_buffer_size;
		descriptor_input += 2;

		Chameleon::Decompress(wrapped_input, Chameleon::DecompressorOutput(output), descriptor_input);
	}
}

#endif // CLOWNLZSS_DECOMPRESSORS_CHAMELEON_H
