#include "saxman.h"

void ClownLZSS_SaxmanDecompress(const ClownLZSS_ReadCallback input_callback, const void* const input_callback_user_data, const ClownLZSS_Callbacks* const output_callbacks, const unsigned int compressed_length)
{
	#define READ_INPUT input_callback((void*)input_callback_user_data)
	#define READ_OUTPUT output_callbacks->read((void*)output_callbacks->user_data)
	#define WRITE_OUTPUT(VALUE) output_callbacks->write((void*)output_callbacks->user_data, VALUE);
	#define SEEK_OUTPUT(POSITION) output_callbacks->seek((void*)output_callbacks->user_data, POSITION);

	unsigned int input_position = 0;
	unsigned int output_position = 0;
	unsigned int descriptor_bits_remaining = 0;
	unsigned char descriptor_bits;

	while (input_position < compressed_length)
	{
		if (descriptor_bits_remaining == 0)
		{
			descriptor_bits_remaining = 8;
			descriptor_bits = READ_INPUT;
			++input_position;
		}

		if ((descriptor_bits & 1) != 0)
		{
			/* Uncompressed. */
			WRITE_OUTPUT(READ_INPUT);
			++input_position;
			++output_position;
		}
		else
		{
			/* Dictionary match. */
			const unsigned int first_byte = READ_INPUT;
			const unsigned int second_byte = READ_INPUT;
			const unsigned int dictionary_index = (first_byte | ((second_byte << 4) & 0xF00)) + (0xF + 3);
			const unsigned int count = (second_byte & 0xF) + 3;
			const unsigned int offset = (output_position - dictionary_index) & 0xFFF;

			unsigned int i;

			if (offset > output_position)
			{
				/* Zero-fill. */
				for (i = 0; i < count; ++i)
					WRITE_OUTPUT(0);
			}
			else
			{
				unsigned char bytes[0xF + 3];

				SEEK_OUTPUT(output_position - offset);

				for (i = 0; i < count; ++i)
					bytes[i] = READ_OUTPUT;

				SEEK_OUTPUT(output_position);

				for (i = 0; i < count; ++i)
				{
					if (i >= offset)
						bytes[i] = bytes[i - offset];

					WRITE_OUTPUT(bytes[i]);
				}
			}

			input_position += 2;
			output_position += count;
		}

		descriptor_bits >>= 1;
		--descriptor_bits_remaining;
	}

	#undef READ_INPUT
	#undef READ_OUTPUT
	#undef WRITE_OUTPUT
	#undef SEEK_OUTPUT
}
