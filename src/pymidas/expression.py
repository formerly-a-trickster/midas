from typing import List
from parser import Token
import tokens as Tok

ASSIGN  = "EXP_ASSIGN"
BINARY  = "EXP_BINARY"
UNARY   = "EXP_UNARY"
CALL    = "EXP_CALL"
IDENT   = "EXP_IDENT"
LITERAL = "EXP_LITERAL"


class Exp(object):
    def __init__(self, kind):
        self.kind = kind

    def accept(self, visitor):
        raise NotImplemented


class Assign(Exp):
    def __init__(self, name: str, exp: Exp):
        super().__init__(ASSIGN)
        self.name = name
        self.exp = exp

    def __str__(self):
        return "( " + self.name + " <- " + str(self.exp) + ") "

    def accept(self, visitor):
        return visitor.visitExpAssign(self)


class Binary(Exp):
    def __init__(self, op: str, left: Exp, right: Exp):
        super().__init__(BINARY)
        self.op = op
        self.left = left
        self.right = right

    def __str__(self):
        return "( " + Tok.to_str[self.op] + " " +\
                str(self.left) + str(self.right) + ") "

    def accept(self, visitor):
        return visitor.visitExpBinary(self)


class Unary(Exp):
    def __init__(self, op: str, exp: Exp):
        super().__init__(UNARY)
        self.op = op
        self.exp = exp

    def __str__(self):
        return "( " + Tok.to_str[self.op] + " " + str(self.exp) + ") "

    def accept(self, visitor):
        return visitor.visitExpUnary(self)


class Call(Exp):
    def __init__(self, callee: Exp, params: List[Exp]):
        super().__init__(CALL)
        self.callee = callee
        self.params = params

    def __str__(self):
        res = "( call " + str(self.callee) + "with "
        for param in self.params:
            res += str(param)
        return res + ") "

    def accept(self, visitor):
        return visitor.visitExpCall(self)


class Ident(Exp):
    def __init__(self, name: str):
        super().__init__(IDENT)
        self.name = name

    def __str__(self):
        return "( ID " + self.name + " ) "

    def accept(self, visitor):
        return visitor.visitExpIdent(self)


class Literal(Exp):
    def __init__(self, tok: Token):
        super().__init__(LITERAL)
        self.tok = tok

    def __str__(self):
        if self.tok.kind == Tok.STRING:
            return "\"" + self.tok.lexeme + "\" "
        else:
            return self.tok.lexeme + " "

    def accept(self, visitor):
        return visitor.visitExpLiteral(self)
