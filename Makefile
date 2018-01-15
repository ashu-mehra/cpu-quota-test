CC=gcc
CFLAGS=-Wall -O3
LIBS=-lpthread
OBJ=main.o
EXE=cputest

$(EXE): $(OBJ) 
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

.PHONY: clean

clean:
	rm -f $(OBJ) $(EXE) 
