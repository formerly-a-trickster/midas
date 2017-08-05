SRC = src/com/jmidas/midas/
AST = src/com/jmidas/tool/
BIN = bin/

JFLAGS = -g -d $(BIN) -cp src/
JC = javac
JI = java

default: astgen
	$(JC) $(JFLAGS) $(SRC)*.java

astgen:
	$(JC) $(JFLAGS) $(AST)*.java
	$(JI) -cp bin/ com.jmidas.tool.GenerateAST $(SRC)

clean:
	rm -r bin/*
	rm src/com/jmidas/lox/Expr.java
