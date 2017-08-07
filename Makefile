SRC = src/
BIN = bin/

default: jmidas cmidas

##################
# Java prototype #
##################
JSRC = $(SRC)com/jmidas/midas/
JAST = $(SRC)com/jmidas/tool/

JFLAGS = -g -d $(BIN) -cp src/
JC = javac
JI = java

jmidas: astgen
	$(JC) $(JFLAGS) $(JSRC)*.java

astgen:
	$(JC) $(JFLAGS) $(JAST)*.java
	$(JI) -cp bin/ com.jmidas.tool.GenerateAST $(JSRC)

####################
# C implementation #
####################
CSRC = src/cmidas

CC     = gcc
CFLAGS = -std=c99 -pedantic -Wextra -Wall -g
SOURCE = main.c lexer.c
OBJ    = $(addprefix ./src/cmidas/, $(SOURCE))
TARGET = $(addprefix $(BIN), cmidas.elf)

cmidas: $(OBJ)
	$(CC) $(OBJ) $(CFLAGS) -o $(TARGET)

clean:
	rm -r bin/*
	rm src/com/jmidas/lox/Expr.java
