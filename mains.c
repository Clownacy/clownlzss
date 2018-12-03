#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "saxman.h"

int main(int argc, char *argv[])
{
	if (argc > 1)
	{
		FILE *file = fopen(argv[1], "rb");

		if (file)
		{
			fseek(file, 0, SEEK_END);
			const size_t file_size = ftell(file);
			rewind(file);

			unsigned char *file_buffer = malloc(file_size);
			fread(file_buffer, 1, file_size, file);
			fclose(file);

			size_t compressed_size;
			unsigned char *compressed_buffer = SaxmanCompress(file_buffer, file_size, &compressed_size);

			FILE *out_file = fopen(argc > 2 ? argv[2] : "out.sax", "wb");

			if (out_file)
			{
				fputc(compressed_size & 0xFF, out_file);
				fputc(compressed_size >> 8, out_file);
				fwrite(compressed_buffer, compressed_size, 1, out_file);
				free(compressed_buffer);
				fclose(out_file);
			}
		}
	}
}
