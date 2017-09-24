BIN = bin
SRC = src/cmidas

CC     = gcc
CFLAGS = -std=c89 -pedantic -Wextra -Wall -g -I$(SRC)/include -O3

MAIN   = $(addprefix $(SRC)/, main.c)
PARSER = $(addprefix $(SRC)/parser/, parser.c lexer.c)
UTIL   = $(addprefix $(SRC)/util/, vector.c)
SOURCE = $(MAIN) $(PARSER) $(UTIL)

TARGET = $(addprefix $(BIN)/, cmidas.elf)

default: cmidas

cmidas: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(SOURCE) $(CFLAGS) -o $(TARGET)

clean:
	rm -r bin/*

.PHONY: cmidas clean
