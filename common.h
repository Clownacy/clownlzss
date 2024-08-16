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

#ifndef CLOWNLZSS_COMMON_H
#define CLOWNLZSS_COMMON_H

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
		concept random_access_input_output_iterator = std::random_access_iterator<T> && std::output_iterator<T, unsigned char>;

		template<typename T>
		class OutputCommon
		{
		public:
			OutputCommon(T output) = delete;
		};

		template<typename T>
		requires Internal::random_access_input_output_iterator<std::decay_t<T>>
		class OutputCommon<T>
		{
		protected:
			std::decay_t<T> output_iterator;

		public:
			OutputCommon(std::decay_t<T> output_iterator)
				: output_iterator(output_iterator)
			{}

			void Write(const unsigned char value)
			{
				*output_iterator = value;
				++output_iterator;
			};
		};

		#if __STDC_HOSTED__ == 1
		template<typename T>
		requires std::is_convertible_v<T&, std::ostream&>
		class OutputCommon<T>
		{
		protected:
			std::ostream &output;

		public:
			OutputCommon(std::ostream &output)
				: output(output)
			{}

			void Write(const unsigned char value)
			{
				output.put(value);
			};
		};
		#endif
	}
}

#endif // CLOWNLZSS_COMMON_H
