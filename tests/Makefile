# The C compilr
CC=clang

# User defined variables. 
# Example: make OTHER_FLAGS="-g -DDEBUG=1"
OTHER_FLAGS=-g

# The compiler flags
CFLAGS=-std=c11 ${OTHER_FLAGS}


# Static object files
STATICS=\
	easykeyv1-clients.o \


# Executable files 
EXECUTABLES=\
	cli-read.out \
	cli-write.out \
	interactive.out \

all: ${EXECUTABLES}

cli-read.out: EasyKeyV1Client-Cli-Read.c $(STATICS)
	$(CC) -o $@ $< $(LINKS) $(CFLAGS) $(STATICS)

cli-write.out: EasyKeyV1Client-Cli-Write.c $(STATICS)
	$(CC) -o $@ $< $(LINKS) $(CFLAGS) $(STATICS)

interactive.out: EasyKeyV1Client-Interactive.c $(STATICS)
	$(CC) -o $@ $< $(LINKS) $(CFLAGS) $(STATICS)

.PHONY: clean

clean:
	rm -f *.o *.so *.out