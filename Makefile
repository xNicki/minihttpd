CC = gcc
CFLAGS = -Wall -Wextra -O2 -Iinclude
OBJ = src/main.o src/server.o src/http.o src/mime.o src/files.o

minihttpd: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

src/%.o: src/%.c include/*.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) minihttpd

.PHONY: clean
