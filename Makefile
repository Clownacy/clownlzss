CXXFLAGS := -std=c++20 -Wall -Wextra -pedantic -Wshift-overflow=2

ifeq (DEBUG,1)
  CXXFLAGS += -Og -ggdb3 -fsanitize=address -fsanitize=undefined -fwrapv
else
  CXXFLAGS += -O2 -flto -s -DNDEBUG
endif

all: clownlzss

clownlzss: main.cpp compressors/clownlzss.c
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $^  $(LIBS)
