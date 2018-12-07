#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chameleon.h"
#include "comper.h"
#include "kosinski.h"
#include "kosinskiplus.h"
#include "rocket.h"
#include "saxman.h"

typedef struct Mode
{
	char *command;
	char *normal_default_filename;
	unsigned char* (*normal_function)(unsigned char *data, size_t data_size, size_t *compressed_size);
	char *moduled_default_filename;
	unsigned char* (*moduled_function)(unsigned char *data, size_t data_size, size_t *compressed_size, size_t module_size);
} Mode;

Mode modes[] = {
	{"-ch", "out.cham", ChameleonCompress, "out.chamm", ModuledChameleonCompress},
	{"-c", "out.comp", ComperCompress, "out.compm", ModuledComperCompress},
	{"-k", "out.kos", KosinskiCompress, "out.kosm", ModuledKosinskiCompress},
	{"-kp", "out.kosp", KosinskiPlusCompress, "out.kospm", ModuledKosinskiPlusCompress},
	{"-r", "out.rock", RocketCompress, "out.rockm", ModuledRocketCompress},
	{"-s", "out.sax", SaxmanCompress, "out.saxm", ModuledSaxmanCompress},
};

int main(int argc, char *argv[])
{
	--argc;
	++argv;

	Mode *mode = NULL;
	char *in_filename = NULL;
	char *out_filename = NULL;
	bool moduled = false;

	for (int i = 0; i < argc; ++i)
	{
		if (argv[i][0] == '-')
		{
			if (!strcmp(argv[i], "-m"))
			{
				moduled = true;
			}
			else
			{
				for (size_t j = 0; j < sizeof(modes) / sizeof(modes[0]); ++j)
				{
					if (!strcmp(argv[i], modes[j].command))
					{
						mode = &modes[j];
						break;
					}
				}
			}
		}
		else
		{
			if (!in_filename)
				in_filename = argv[i];
			else
				out_filename = argv[i];
		}
	}

	if (!in_filename)
	{
		printf("Error: Input file not specified\n");
	}
	else if (!mode)
	{
		printf("Error: Mode not specified\n");
	}
	else
	{
		if (!out_filename)
			out_filename = moduled ? mode->moduled_default_filename : mode->normal_default_filename;

		FILE *in_file = fopen(in_filename, "rb");

		if (in_file)
		{
			fseek(in_file, 0, SEEK_END);
			const size_t file_size = ftell(in_file);
			rewind(in_file);

			unsigned char *file_buffer = malloc(file_size);
			fread(file_buffer, 1, file_size, in_file);
			fclose(in_file);

			size_t compressed_size;
			unsigned char *compressed_buffer;

			if (moduled)
				compressed_buffer = mode->moduled_function(file_buffer, file_size, &compressed_size, 0x1000);
			else
				compressed_buffer = mode->normal_function(file_buffer, file_size, &compressed_size);

			FILE *out_file = fopen(out_filename, "wb");

			if (out_file)
			{
				fwrite(compressed_buffer, compressed_size, 1, out_file);
				free(compressed_buffer);
				fclose(out_file);
			}
		}
	}
}
