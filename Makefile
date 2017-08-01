SRC = src/com/craftinginterpreters/lox/
AST = src/com/craftinginterpreters/tool/
BIN = bin/

JFLAGS = -g -d $(BIN)
JC = javac
JI = java

default: astgen
	$(JC) $(JFLAGS) $(SRC)*.java

astgen:
	$(JC) $(JFLAGS) $(AST)*.java
	$(JI) -cp bin/ com.craftinginterpreters.tool.GenerateAST $(SRC)

clean:
	rm -r bin/*
	rm src/com/craftinginterpreters/lox/Expr.java