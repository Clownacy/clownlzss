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
