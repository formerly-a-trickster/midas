#!/usr/bin/env python3
from sys import argv
from parser import Parser

if __name__ == "__main__":
    args = argv[1:]

    if len(args) == 1:
        par = Parser()
        ast = par.parse(args[0])
        for stm in ast:
            print(stm, end="")
    else:
        print("Usage: pymidas [script]")
