/*
	(C) 2018-2022 Clownacy

	This software is provided 'as-is', without any express or implied
	warranty.  In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not
	   claim that you wrote the original software. If you use this software
	   in a product, an acknowledgment in the product documentation would be
	   appreciated but is not required.
	2. Altered source versions must be plainly marked as such, and must not be
	   misrepresented as being the original software.
	3. This notice may not be removed or altered from any source distribution.
*/

#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>

#include "clowncommon.h"

typedef struct ClownLZSS_Callbacks
{
	void *user_data;
	void (*write)(void *user_data, unsigned char byte);
	void (*seek)(void *user_data, size_t position);
	size_t (*tell)(void *user_data);
} ClownLZSS_Callbacks;

cc_bool ClownLZSS_ModuledCompressionWrapper(const unsigned char *data, size_t data_size, const ClownLZSS_Callbacks *callbacks, cc_bool (*compression_function)(const unsigned char *data, size_t data_size, const ClownLZSS_Callbacks *callbacks), size_t module_size, size_t module_alignment);

#endif /* COMMON_H */
