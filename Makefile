CFLAGS := -O2 -std=c99 -s -Wall -Wextra -pedantic -fno-ident -flto

all: tool

tool: main.c memory_stream.c chameleon.c common.c comper.c kosinski.c kosinskiplus.c rocket.c saxman.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)
