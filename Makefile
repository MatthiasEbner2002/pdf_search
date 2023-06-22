CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 `pkg-config --cflags poppler-glib` -fopenmp -O3
LIBS = `pkg-config --libs poppler-glib` -lm

all: pdf_search

pdf_search: pdf_search.c
	$(CC) $(CFLAGS) $< -o $@ $(LIBS)

clean:
	rm -f pdf_search
