#pragma once

#include <stddef.h>

void ClownLZSS_CompressData(unsigned char *data, size_t data_size, size_t max_match_length, size_t max_match_distance, size_t literal_cost, unsigned int (*cost_callback)(size_t distance, size_t length), void (*literal_callback)(unsigned char value), void (*match_callback)(size_t longest_match_index, size_t longest_match_length));
