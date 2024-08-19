/*
Copyright (c) 2018-2022 Clownacy

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

#ifndef CLOWNLZSS_COMPRESSORS_COMMON_H
#define CLOWNLZSS_COMPRESSORS_COMMON_H

#include "../common.h"

#include <iterator>
#if __STDC_HOSTED__ == 1
	#include <ostream>
#endif
#include <type_traits>

namespace ClownLZSS
{
	namespace Internal
	{
		template<typename T>
		bool ModuledCompressionWrapper(const unsigned char* const data, const std::size_t data_size, T &&output, bool (* const compression_function)(const unsigned char *data, std::size_t data_size, T &&output), const std::size_t module_size, const std::size_t module_alignment)
		{
			const unsigned int header = (data_size % module_size) | ((data_size / module_size) << 12);

			output.Write((header >> (8 * 1)) & 0xFF);
			output.Write((header >> (8 * 0)) & 0xFF);

			typename T::difference_type compressed_size = 0;
			for (std::size_t i = 0; i < data_size; i += module_size)
			{
				if (compressed_size % module_alignment != 0)
					output.Fill(0, module_alignment - (compressed_size % module_alignment));

				const auto start_position = output.Tell();

				if (!compression_function(data + i, module_size < data_size - i ? module_size : data_size - i, std::forward<T>(output)))
					return false;

				compressed_size = output.Distance(start_position);
			}

			return true;
		}
	}

	// CompressorOutput

	template<typename T>
	class CompressorOutput
	{
	public:
		CompressorOutput(T output) = delete;
	};

	template<typename T>
	requires Internal::random_access_input_output_iterator<std::decay_t<T>>
	class CompressorOutput<T> : public Internal::OutputCommon<T>
	{
	public:
		using Internal::OutputCommon<T>::OutputCommon;
	};

	#if __STDC_HOSTED__ == 1
	template<typename T>
	requires std::is_convertible_v<T&, std::ostream&>
	class CompressorOutput<T> : public Internal::OutputCommon<T>
	{
	public:
		using Internal::OutputCommon<T>::OutputCommon;
	};
	#endif
}

#endif // CLOWNLZSS_COMPRESSORS_COMMON_H
