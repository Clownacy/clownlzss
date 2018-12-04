#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "comper.h"

#define STB_LEAKCHECK_IMPLEMENTATION
#include "stb_leakcheck.h"

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
			unsigned char *compressed_buffer = ComperCompress(file_buffer, file_size, &compressed_size);

			FILE *out_file = fopen(argc > 2 ? argv[2] : "out.comp", "wb");

			if (out_file)
			{
				fwrite(compressed_buffer, compressed_size, 1, out_file);
				free(compressed_buffer);
				fclose(out_file);
			}
		}
	}

	stb_leakcheck_dumpmem();
}
