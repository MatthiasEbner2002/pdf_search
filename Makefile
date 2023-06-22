CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 `pkg-config --cflags poppler-glib`
LIBS = `pkg-config --libs poppler-glib` -lm

all: pdf_search_file

pdf_search_file: pdf_search_file.c
	$(CC) $(CFLAGS) $< -o $@ $(LIBS)

clean:
	rm -f pdf_search_file
