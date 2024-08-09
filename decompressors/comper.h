#ifndef CLOWNLZSS_DECOMPRESSORS_COMPER_H
#define CLOWNLZSS_DECOMPRESSORS_COMPER_H

/* C template wheeeeeeeee */
#define CLOWNLZSS_COMPER_DECOMPRESS \
{ \
	unsigned int descriptor_bits_remaining = 0; \
	unsigned short descriptor_bits; \
 \
	for (;;) \
	{ \
		if (descriptor_bits_remaining == 0) \
		{ \
			descriptor_bits_remaining = 16; \
			descriptor_bits = (unsigned short)(CLOWNLZSS_READ_INPUT) << 8; \
			descriptor_bits |= (CLOWNLZSS_READ_INPUT); \
		} \
 \
		if ((descriptor_bits & 0x8000) == 0) \
		{ \
			/* Uncompressed. */ \
			CLOWNLZSS_WRITE_OUTPUT((CLOWNLZSS_READ_INPUT)); \
			CLOWNLZSS_WRITE_OUTPUT((CLOWNLZSS_READ_INPUT)); \
		} \
		else \
		{ \
			/* Dictionary match. */ \
			const unsigned int raw_offset = (CLOWNLZSS_READ_INPUT); \
			const unsigned int offset = (0x100 - raw_offset) * 2; \
			const unsigned int raw_count = (CLOWNLZSS_READ_INPUT); \
			const unsigned int count = (raw_count + 1) * 2; \
 \
			if (raw_count == 0) \
				break; \
 \
			CLOWNLZSS_COPY_OUTPUT(offset, count, (0xFF + 1) * 2); \
		} \
 \
		descriptor_bits <<= 1; \
		--descriptor_bits_remaining; \
	} \
}

#ifdef __cplusplus
#include <algorithm>

template<typename T1, typename T2>
void ClownLZSS_ComperDecompress(T1 input_iterator, T2 output_iterator)
{
	#define CLOWNLZSS_READ_INPUT (++input_iterator, input_iterator[-1])
	#define CLOWNLZSS_WRITE_OUTPUT(VALUE) (*output_iterator = (VALUE), ++output_iterator)
	#define CLOWNLZSS_COPY_OUTPUT(OFFSET, COUNT, MAXIMUM_COUNT) std::copy(output_iterator - (OFFSET), output_iterator - (OFFSET) + (COUNT), output_iterator)
	CLOWNLZSS_COMPER_DECOMPRESS;
	#undef CLOWNLZSS_READ_INPUT
	#undef CLOWNLZSS_WRITE_OUTPUT
	#undef CLOWNLZSS_COPY_OUTPUT
}
#endif

#endif /* CLOWNLZSS_DECOMPRESSORS_COMPER_H */
