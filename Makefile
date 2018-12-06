CFLAGS := -O2 -std=c11 -s -Wall -Wextra -pedantic -fno-ident -flto

all: cham comp kos kosp rock sax

cham: main.c chameleon.c memory_stream.c
	$(CC) $(CFLAGS) -DCHAMELEON -o $@ $^ $(LDFLAGS) $(LIBS)

comp: main.c comper.c memory_stream.c
	$(CC) $(CFLAGS) -DCOMPER -o $@ $^ $(LDFLAGS) $(LIBS)

kos: main.c kosinski.c memory_stream.c
	$(CC) $(CFLAGS) -DKOSINSKI -o $@ $^ $(LDFLAGS) $(LIBS)

kosp: main.c kosinskiplus.c memory_stream.c
	$(CC) $(CFLAGS) -DKOSINSKIPLUS -o $@ $^ $(LDFLAGS) $(LIBS)

rock: main.c rocket.c memory_stream.c
	$(CC) $(CFLAGS) -DROCKET -o $@ $^ $(LDFLAGS) $(LIBS)

sax: main.c saxman.c memory_stream.c
	$(CC) $(CFLAGS) -DSAXMAN -o $@ $^ $(LDFLAGS) $(LIBS)
