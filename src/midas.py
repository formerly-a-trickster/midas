#!/usr/bin/env python3
import sys
import lexer as Lex
import parser as Par
import interpreter as Int
import resolver as Res


if __name__ == "__main__":
    args = sys.argv[1:]
    par = Par.Parser()
    int = Int.Interpreter()
    res = Res.Resolver(int)

    if len(args) == 0:
        while True:
            try:
                line = input("=> ") + "\0"
            except KeyboardInterrupt:
                sys.exit(1)

            try:
                ast = par.parse(line)
                int.interpret(ast)
            except Lex.LexerError as err:
                print("[line %s] %s" % (err.lineno, err.msg))
            except Par.ParserError as err:
                print("[line %s] %s" % (err.lineno, err.msg))
    elif len(args) == 1:
        try:
            with open(args[0], "r") as source:
                text = source.read() + "\0"
            ast = par.parse(text)
            res.resolveAst(ast)
            int.interpret(ast)
        except Lex.LexerError as err:
            print("[line %s] %s" % (err.lineno, err.msg))
        except Par.ParserError as err:
            print("[line %s] %s" % (err.lineno, err.msg))
    else:
        print("Usage: pymidas [script]")
