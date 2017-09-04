#!/usr/bin/env python3
from sys import argv
from parser import Parser
from interpreter import Interpreter

if __name__ == "__main__":
    args = argv[1:]

    if len(args) == 1:
        par = Parser()
        int = Interpreter()
        ast = par.parse(args[0])
        int.interpret(ast)
    else:
        print("Usage: pymidas [script]")
