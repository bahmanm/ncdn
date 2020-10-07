P = ncdn
OBJECTS =

CC = clang
CFLAGS = -g -O3 -std=gnu11 \
	 -Wall -Wextra -Wpedantic \
	 -Wformat=2 -Wno-unused-parameter -Wshadow \
         -Wwrite-strings -Wstrict-prototypes -Wold-style-definition \
         -Wredundant-decls -Wnested-externs -Wmissing-include-dirs \
	 -lgc -ljansson
LDFLAGS =

.PHONY : all clean build build-image run

all : $(P)

clean :
	rm -rf *.o $(P) $(P)*SYM

$(P) : $(OBJECTS)
