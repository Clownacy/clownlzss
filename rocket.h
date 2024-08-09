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

#ifndef CLOWNLZSS_ROCKET_H
#define CLOWNLZSS_ROCKET_H

#include <stddef.h>

#include "clowncommon/clowncommon.h"

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

cc_bool ClownLZSS_RocketCompress(const unsigned char *data, size_t data_size, const ClownLZSS_Callbacks *callbacks);

#ifdef __cplusplus
}
#endif

#endif /* CLOWNLZSS_ROCKET_H */
