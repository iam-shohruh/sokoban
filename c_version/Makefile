CC = gcc
CFLAGS = -O2 -std=c11 -Wall -Wextra
TARGET = sokoban.exe
SOURCES = GRAPH_SEARCH.c HashTable.c Standart_Search.c SpecificToProblem.c

all: $(TARGET)

$(TARGET): $(SOURCES) GRAPH_SEARCH.h HashTable.h data_types.h
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET)

clean:
	rm -f $(TARGET)
