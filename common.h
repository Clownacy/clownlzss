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
