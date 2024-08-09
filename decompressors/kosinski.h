#ifndef CLOWNLZSS_DECOMPRESSORS_KOSINSKI_H
#define CLOWNLZSS_DECOMPRESSORS_KOSINSKI_H

#include <algorithm>

template<typename T1, typename T2>
void ClownLZSS_KosinskiDecompress(T1 input_iterator, T2 output_iterator)
{
	unsigned int descriptor_bits_remaining;
	unsigned short descriptor_bits;

	const auto GetDescriptor = [&]()
	{
		const unsigned int low_byte = *input_iterator; ++input_iterator;
		const unsigned int high_byte = *input_iterator; ++input_iterator;

		descriptor_bits = (high_byte << 8) | low_byte;

		descriptor_bits_remaining = 16;

	};

	const auto PopDescriptor = [&]()
	{
		const bool result = (descriptor_bits & 1) != 0;

		descriptor_bits >>= 1;

		if (--descriptor_bits_remaining == 0)
			GetDescriptor();

		return result;
	};

	GetDescriptor();

	for (;;)
	{
		if (PopDescriptor())
		{
			const unsigned char byte = *input_iterator; ++input_iterator;
			*output_iterator = byte; ++output_iterator;
		}
		else
		{
			unsigned int distance;
			size_t count;

			if (PopDescriptor())
			{
				const unsigned char low_byte = *input_iterator; ++input_iterator;
				const unsigned char high_byte = *input_iterator; ++input_iterator;

				distance = ((high_byte & 0xF8) << 5) | low_byte;
				distance = (distance ^ 0x1FFF) + 1; // Convert from negative two's-complement to positive.
				count = high_byte & 7;

				if (count != 0)
				{
					count += 2;
				}
				else
				{
					count = *input_iterator + 1; ++input_iterator;

					if (count == 1)
						break;
					else if (count == 2)
						continue;
				}
			}
			else
			{
				count = 2;

				if (PopDescriptor())
					count += 2;
				if (PopDescriptor())
					count += 1;

				distance = 0x100 - *input_iterator; ++input_iterator; // Convert from negative two's-complement to positive.
			}

			std::copy(output_iterator - distance, output_iterator - distance + count, output_iterator);
		}
	}
}

#endif /* CLOWNLZSS_DECOMPRESSORS_KOSINSKI_H */
