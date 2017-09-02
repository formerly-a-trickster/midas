BIN = bin
SRC = src/cmidas

CC     = gcc
CFLAGS = -std=c89 -pedantic -Wextra -Wall -g -I$(SRC)/include -O3

MAIN   = $(addprefix $(SRC)/, main.c)
PARSER = $(addprefix $(SRC)/parser/, parser.c lexer.c)
INTPR  = $(addprefix $(SRC)/interpreter/, interpreter.c environ.c value.c)
UTIL   = $(addprefix $(SRC)/util/, hash.c vector.c)
SOURCE = $(MAIN) $(PARSER) $(INTPR) $(UTIL)

TARGET = $(addprefix $(BIN)/, cmidas.elf)

default: cmidas

cmidas: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(SOURCE) $(CFLAGS) -o $(TARGET)

clean:
	rm -r bin/*

.PHONY: cmidas clean
