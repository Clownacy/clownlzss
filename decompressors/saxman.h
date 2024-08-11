#ifndef CLOWNLZSS_DECOMPRESSORS_SAXMAN_H
#define CLOWNLZSS_DECOMPRESSORS_SAXMAN_H

/* C template wheeeeeeeee */
#define CLOWNLZSS_SAXMAN_DECOMPRESS(COMPRESSED_LENGTH) \
{ \
	unsigned int input_position = 0; \
	unsigned int output_position = 0; \
	unsigned int descriptor_bits_remaining = 0; \
	unsigned char descriptor_bits; \
 \
	while (input_position < (COMPRESSED_LENGTH)) \
	{ \
		if (descriptor_bits_remaining == 0) \
		{ \
			descriptor_bits_remaining = 8; \
			descriptor_bits = (CLOWNLZSS_READ_INPUT); \
			++input_position; \
		} \
 \
		if ((descriptor_bits & 1) != 0) \
		{ \
			/* Uncompressed. */ \
			CLOWNLZSS_WRITE_OUTPUT((CLOWNLZSS_READ_INPUT)); \
			++input_position; \
			++output_position; \
		} \
		else \
		{ \
			/* Dictionary match. */ \
			const unsigned int first_byte = (CLOWNLZSS_READ_INPUT); \
			const unsigned int second_byte = (CLOWNLZSS_READ_INPUT); \
			const unsigned int dictionary_index = (first_byte | ((second_byte << 4) & 0xF00)) + (0xF + 3); \
			const unsigned int count = (second_byte & 0xF) + 3; \
			const unsigned int offset = (output_position - dictionary_index) & 0xFFF; \
 \
			if (offset > output_position) \
			{ \
				/* Zero-fill. */ \
				CLOWNLZSS_FILL_OUTPUT(0, count, 0xF + 3); \
			} \
			else \
			{ \
				/* Copy */ \
				CLOWNLZSS_COPY_OUTPUT(offset, count, 0xF + 3); \
			} \
 \
			input_position += 2; \
			output_position += count; \
		} \
 \
		descriptor_bits >>= 1; \
		--descriptor_bits_remaining; \
	} \
}

#ifdef __cplusplus

#include "common.h"

namespace ClownLZSS
{
	namespace Internal
	{
		template<typename T1, typename T2>
		void SaxmanDecompress(T1 &&input, T2 &&output, const unsigned int compressed_length)
		{
			#define CLOWNLZSS_READ_INPUT Read(input)
			#define CLOWNLZSS_WRITE_OUTPUT(VALUE) output.Write((VALUE))
			#define CLOWNLZSS_FILL_OUTPUT(VALUE, COUNT, MAXIMUM_COUNT) output.Fill((VALUE), (COUNT))
			#define CLOWNLZSS_COPY_OUTPUT(OFFSET, COUNT, MAXIMUM_COUNT) output.template Copy<0xF + 3>((OFFSET), (COUNT))
			CLOWNLZSS_SAXMAN_DECOMPRESS(compressed_length);
			#undef CLOWNLZSS_READ_INPUT
			#undef CLOWNLZSS_WRITE_OUTPUT
			#undef CLOWNLZSS_FILL_OUTPUT
			#undef CLOWNLZSS_COPY_OUTPUT
		}
	}

	template<std::input_iterator T1, Internal::random_access_input_output_iterator T2>
	inline void SaxmanDecompress(T1 input_begin, T1 input_end, T2 output_begin)
	{
		Internal::SaxmanDecompress(input_begin, Output(output_begin), input_end - input_begin);
	}

	#if __STDC_HOSTED__ == 1
	inline void SaxmanDecompress(std::istream &input, std::iostream &output, const unsigned int compressed_length)
	{
		Internal::SaxmanDecompress(input, Output(output), compressed_length);
	}
	#endif
}

#endif

#endif /* CLOWNLZSS_DECOMPRESSORS_SAXMAN_H */
