NAME=cgrip
CC=gcc
CFLAGS=$(shell pkg-config --cflags libarchive libcurl) -ansi -Wall -pedantic -g -DCGRIP_TERMCOLOR
LDFLAGS=$(shell pkg-config --libs libarchive libcurl)
OBJECTS=$(NAME).o cgapi.o lodepng.o

.PHONY: all depend clean

all: depend $(NAME)

$(NAME): $(OBJECTS)

depend:
	gcc -MM $(foreach obj,$(OBJECTS),$(basename $(obj)).c) > Makefile.dep

clean:
	rm -f $(NAME) $(OBJECTS)

-include Makefile.dep

