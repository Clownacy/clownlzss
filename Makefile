CFLAGS := -ansi -Wall -Wextra -pedantic -Wshift-overflow=2

ifeq (DEBUG,1)
  CFLAGS += -Og -ggdb3 -fsanitize=address -fsanitize=undefined -fwrapv
else
  CFLAGS += -O2 -flto -s -DNDEBUG
endif

all: clownlzss

clownlzss: main.c clownlzss.c chameleon.c common.c comper.c faxman.c kosinski.c kosinskiplus.c rage.c rocket.c saxman.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $^  $(LIBS)
