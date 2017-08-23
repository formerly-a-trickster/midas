BIN = bin
SRC = src/cmidas

CC     = gcc
CFLAGS = -std=c99 -pedantic -Wextra -Wall -g -I$(SRC)/include

MAIN   = $(addprefix $(SRC)/, main.c)
PARSER = $(addprefix $(SRC)/parser/, parser.c lexer.c)
INTPR  = $(addprefix $(SRC)/interpreter/, interpreter.c env.c)
UTIL   = $(addprefix $(SRC)/util/, error.c hash.c vector.c)
SOURCE = $(MAIN) $(PARSER) $(INTPR) $(UTIL)

TARGET = $(addprefix $(BIN)/, cmidas.elf)

default: cmidas

cmidas: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(SOURCE) $(CFLAGS) -o $(TARGET)

clean:
	rm -r bin/*

.PHONY: cmidas clean
