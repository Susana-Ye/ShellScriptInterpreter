CC=gcc
FLAGS=-Wno-implicit-function-declaration
CFLAGS=-I.
###############################

OBJ1 = scripter.o
OBJ2 = process_line.o
OBJ3 = mygrep.o

all: scripter mygrep

%.o: %.c
	$(CC) $(FLAGS) -c -o $@ $< $(CFLAGS)

scripter: $(OBJ1) $(OBJ2)
	$(CC) $(FLAGS) -o $@ $^ $(LIBS)

mygrep: $(OBJ3)
	$(CC) $(FLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f *.o scripter mygrep
