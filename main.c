#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef CHAMELEON
#include "chameleon.h"
#define FUNCTION ChameleonCompress
#define EXTENSION "cham"
#endif
#ifdef COMPER
#include "comper.h"
#define FUNCTION ComperCompress
#define EXTENSION "comp"
#endif
#ifdef KOSINSKI
#include "kosinski.h"
#define FUNCTION KosinskiCompress
#define EXTENSION "kos"
#endif
#ifdef KOSINSKIPLUS
#include "kosinskiplus.h"
#define FUNCTION KosinskiPlusCompress
#define EXTENSION "kosp"
#endif
#ifdef ROCKET
#include "rocket.h"
#define FUNCTION RocketCompress
#define EXTENSION "rock"
#endif
#ifdef SAXMAN
#include "saxman.h"
#define FUNCTION SaxmanCompress
#define EXTENSION "sax"
#endif

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

			unsigned char *compressed_buffer = FUNCTION(file_buffer, file_size, &compressed_size);

			FILE *out_file = fopen(argc > 2 ? argv[2] : "out." EXTENSION, "wb");

			if (out_file)
			{
				#ifdef ROCKET
				fputc(file_size >> 8, out_file);
				fputc(file_size & 0xFF, out_file);
				fputc(compressed_size >> 8, out_file);
				fputc(compressed_size & 0xFF, out_file);
				#endif
				#ifdef SAXMAN
				fputc(compressed_size & 0xFF, out_file);
				fputc(compressed_size >> 8, out_file);
				#endif
				fwrite(compressed_buffer, compressed_size, 1, out_file);
				free(compressed_buffer);
				fclose(out_file);
			}
		}
	}
}
