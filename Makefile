CFLAGS := -O2 -s -Wall -Wextra -fno-ident -flto

all: kos comp

kos: maink.c memory_stream.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

comp: main.c memory_stream.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)
