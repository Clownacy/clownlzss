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

#ifndef CLOWNLZSS_COMPRESSORS_NEW_COMMON_H
#define CLOWNLZSS_COMPRESSORS_NEW_COMMON_H

#include "../../common.h"

#if __STDC_HOSTED__ == 1
	#include <ostream>
#endif
#include <type_traits>

namespace ClownLZSS
{
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
		using pos_type = std::decay_t<T>;

		using Internal::OutputCommon<T>::output_iterator;
		using Internal::OutputCommon<T>::OutputCommon;

		pos_type Tell()
		{
			return output_iterator;
		};

		void Seek(const pos_type &position)
		{
			output_iterator = position;
		};
	};

	#if __STDC_HOSTED__ == 1
	template<typename T>
	requires std::is_convertible_v<T&, std::ostream&>
	class CompressorOutput<T> : public Internal::OutputCommon<T>
	{
	public:
		using pos_type = std::ostream::pos_type;

		using Internal::OutputCommon<T>::output;
		using Internal::OutputCommon<T>::OutputCommon;

		pos_type Tell()
		{
			return output.tellp();
		};

		void Seek(const pos_type &position)
		{
			output.seekp(position);
		};
	};
	#endif
}

#endif // CLOWNLZSS_COMPRESSORS_NEW_COMMON_H
