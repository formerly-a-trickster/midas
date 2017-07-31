SRC = src/com/craftinginterpreters/lox/
BIN = bin/

JFLAGS = -g -d $(BIN)
JC = javac

OBJECTS = Lox.java Scanner.java Token.java TokenType.java 

default:
	$(JC) $(JFLAGS) $(SRC)*.java

clean:
	rm -r bin/*