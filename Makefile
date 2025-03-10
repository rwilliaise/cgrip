NAME=cgrip
CC=gcc
CFLAGS=$(shell pkg-config --cflags libarchive libcurl) -ansi -Wall -pedantic -g -DCGRIP_TERMCOLOR
LDFLAGS=$(shell pkg-config --libs libarchive libcurl) -lm
OBJECTS=$(NAME).o cgapi.o cgpro.o lodepng.o gen_godot4.o

.PHONY: all depend clean

all: depend $(NAME)

$(NAME): $(OBJECTS)

depend:
	gcc -MM $(foreach obj,$(OBJECTS),$(basename $(obj)).c) > Makefile.dep

clean:
	rm -f $(NAME) $(OBJECTS)

-include Makefile.dep

