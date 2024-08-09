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
			descriptor_bits = (CLOWNLZSS_SAXMAN_DECOMPRESS_READ_INPUT); \
			++input_position; \
		} \
 \
		if ((descriptor_bits & 1) != 0) \
		{ \
			/* Uncompressed. */ \
			CLOWNLZSS_SAXMAN_DECOMPRESS_WRITE_OUTPUT((CLOWNLZSS_SAXMAN_DECOMPRESS_READ_INPUT)); \
			++input_position; \
			++output_position; \
		} \
		else \
		{ \
			/* Dictionary match. */ \
			const unsigned int first_byte = (CLOWNLZSS_SAXMAN_DECOMPRESS_READ_INPUT); \
			const unsigned int second_byte = (CLOWNLZSS_SAXMAN_DECOMPRESS_READ_INPUT); \
			const unsigned int dictionary_index = (first_byte | ((second_byte << 4) & 0xF00)) + (0xF + 3); \
			const unsigned int count = (second_byte & 0xF) + 3; \
			const unsigned int offset = (output_position - dictionary_index) & 0xFFF; \
 \
			if (offset > output_position) \
			{ \
				/* Zero-fill. */ \
				CLOWNLZSS_SAXMAN_DECOMPRESS_FILL_OUTPUT(0, count); \
			} \
			else \
			{ \
				/* Copy */ \
				CLOWNLZSS_SAXMAN_DECOMPRESS_COPY_OUTPUT(offset, count); \
			} \
 \
			input_position += 2; \
			output_position += count; \
		} \
 \
		descriptor_bits >>= 1; \
		--descriptor_bits_remaining; \
	} \
 \
}

#if defined(__cplusplus)
#include <algorithm>

template<typename T1, typename T2>
void ClownLZSS_SaxmanDecompress(T1 input_begin, T1 input_end, T2 output_begin)
{
	T1 input_iterator = input_begin;
	T2 output_iterator = output_begin;
	#define CLOWNLZSS_SAXMAN_DECOMPRESS_READ_INPUT (++input_iterator, input_iterator[-1])
	#define CLOWNLZSS_SAXMAN_DECOMPRESS_WRITE_OUTPUT(VALUE) (*output_iterator = (VALUE), ++output_iterator)
	#define CLOWNLZSS_SAXMAN_DECOMPRESS_FILL_OUTPUT(VALUE, COUNT) std::fill_n(output_iterator, (COUNT), (VALUE))
	#define CLOWNLZSS_SAXMAN_DECOMPRESS_COPY_OUTPUT(OFFSET, COUNT) std::copy(output_iterator - (OFFSET), output_iterator - (OFFSET) + (COUNT), output_iterator)
	CLOWNLZSS_SAXMAN_DECOMPRESS(input_end - input_iterator);
	#undef CLOWNLZSS_SAXMAN_DECOMPRESS_READ_INPUT
	#undef CLOWNLZSS_SAXMAN_DECOMPRESS_WRITE_OUTPUT
	#undef CLOWNLZSS_SAXMAN_DECOMPRESS_FILL_OUTPUT
	#undef CLOWNLZSS_SAXMAN_DECOMPRESS_COPY_OUTPUT
}
#endif

#endif /* CLOWNLZSS_DECOMPRESSORS_SAXMAN_H */
