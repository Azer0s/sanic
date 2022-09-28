CC=gcc
CCFLAGS=
FILES=main.c http.c

.PHONY: clear

all:
	$(MAKE) clear
	mkdir bin || true
	$(CC) $(CCFLAGS) $(FILES) -o bin/main

clear: 
	rm -rf bin/ || true
