CFLAGS := -O2 -s -Wall -Wextra -fno-ident -flto

all: kos

kos: main.c memory_stream.c clownlzss.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)
