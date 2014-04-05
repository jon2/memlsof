# Generic Makefile
# Set the "SOURCES" and the "EXECUTABLE" variables to whatever is needed.  Recycle often.

CC=gcc
CFLAGS=-g -Wall -c -I.
LDFLAGS=
SOURCES=memlsof.c memlsof_utils.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=memlsof

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf *.o memlsof *~

