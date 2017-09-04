from typing import List, Type
from parser import Token
from toks import *

EXP_ASSIGN  = "EXP_ASSIGN"
EXP_BINARY  = "EXP_BINARY"
EXP_UNARY   = "EXP_UNARY"
EXP_CALL    = "EXP_CALL"
EXP_IDENT   = "EXP_IDENT"
EXP_LITERAL = "EXP_LITERAL"

class Exp(object):
    def __init__(self, kind):
        self.kind = kind

class ExpAssign(Exp):
    def __init__(self, name: str, exp: Exp):
        super().__init__(EXP_ASSIGN)
        self.name = name
        self.exp = exp

    def __str__(self):
        return "( " + self.name + " <- " + str(self.exp) + ") "

class ExpBinary(Exp):
    def __init__(self, op: str, left: Exp, right: Exp):
        super().__init__(EXP_BINARY)
        self.op = op
        self.left = left
        self.right = right

    def __str__(self):
        return "( " + tok_str[self.op] + " " +\
                str(self.left) + str(self.right) + ") "

class ExpUnary(Exp):
    def __init__(self, op: str, exp: Exp):
        super().__init__(EXP_UNARY)
        self.op = op
        self.exp = exp

    def __str__(self):
        return "( " + tok_str[self.op] + " " + str(self.exp) + ") "

class ExpCall(Exp):
    def __init__(self, callee: Exp, params: List[Exp]):
        super().__init__(EXP_CALL)
        self.callee = callee
        self.params = params

    def __str__(self):
        res = "( call " + str(self.callee) + "with "
        for param in self.params:
            res += str(param)
        return res + ") "

class ExpIdent(Exp):
    def __init__(self, name: str):
        super().__init__(EXP_IDENT)
        self.name = name

    def __str__(self):
        return "( ID " + self.name + " ) "

class ExpLiteral(Exp):
    def __init__(self, tok: Token):
        super().__init__(EXP_LITERAL)
        self.tok = tok

    def __str__(self):
        if self.tok.kind == TOK_STRING:
            return "\"" + self.tok.lexeme + "\" "
        else:
            return self.tok.lexeme + " "

