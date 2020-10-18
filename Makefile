P = ncdn
OBJECTS =

CC = clang
CFLAGS = -g -O3 -std=gnu11 \
	 -Wall -Wextra -Wpedantic \
	 -Wformat=2 -Wno-unused-parameter -Wshadow \
         -Wwrite-strings -Wstrict-prototypes -Wold-style-definition \
         -Wredundant-decls -Wnested-externs -Wmissing-include-dirs \
	 -lgc \
	 $$(pkg-config --cflags jansson glib-2.0)
LDFLAGS = $$(pkg-config --libs jansson glib-2.0)

.PHONY : all clean build build-image run

all : $(P)

clean :
	rm -rf *.o $(P) $(P)*SYM

$(P) : $(OBJECTS)
