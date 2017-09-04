from typing import List
import expression as E

VAR_DECL = "STM_VAR_DECL"
FUN_DECL = "STM_FUN_DECL"
BLOCK    = "STM_BLOCK"
IF       = "STM_IF"
WHILE    = "STM_WHILE"
BREAK    = "STM_BREAK"
RETURN   = "STM_RETURN"
PRINT    = "STM_PRINT"
EXP_STM  = "STM_EXP_STM"


class Stm(object):
    def __init__(self, kind):
        self.kind = kind

    def accept(self, visitor):
        raise NotImplemented


class VarDecl(Stm):
    def __init__(self, name: str, exp: E.Exp):
        super().__init__(VAR_DECL)
        self.name = name
        self.exp = exp

    def __str__(self, i=0):
        return " " * i + "[ var " + self.name + " as " + str(self.exp) + "]\n"

    def accept(self, visitor):
        return visitor.visitStmVarDecl(self)


class FunDecl(Stm):
    def __init__(self, name: str, formals: List[str], body: Stm):
        super().__init__(FUN_DECL)
        self.name = name
        self.formals = formals
        self.body = body

    def __str__(self, i=0):
        res = " " * i + "[ fun " + self.name + "< "
        for arg in self.formals:
            res += arg + " "
        res += ">\n"
        res += self.body.__str__(i)
        return res

    def accept(self, visitor):
        return visitor.visitStmFunDecl(self)


class Block(Stm):
    def __init__(self, block: List[Stm]):
        super().__init__(BLOCK)
        self.block = block

    def __str__(self, i=0):
        res = " " * i + "[\n"
        for stm in self.block:
            res += stm.__str__(i + 4)
        res += " " * i + "]\n"
        return res

    def accept(self, visitor):
        return visitor.visitStmBlock(self)


class If(Stm):
    def __init__(self, cond: E.Exp, then_block: Stm, else_block: Stm):
        super().__init__(IF)
        self.cond = cond
        self.then_block = then_block
        self.else_block = else_block

    def __str__(self, i=0):
        res = " " * i + "[ if " + str(self.cond) + "\n"
        res += " " * (i + 2) + "then\n"
        res += self.then_block.__str__(i + 4)
        if self.else_block != None:
            res += " " * (i + 2) + "else\n"
            res += self.else_block.__str__(i + 4)
        res += " " * i + "]\n"
        return res

    def accept(self, visitor):
        return visitor.visitStmIf(self)


class While(Stm):
    def __init__(self, cond: E.Exp, body: Stm):
        super().__init__(WHILE)
        self.cond = cond
        self.body = body

    def __str__(self, i=0):
        res = " " * i + "[ while " + str(self.cond) + "\n"
        res += " " * (i + 2) + "loop\n"
        res += self.body.__str__(i + 4)
        res += " " * i + "]\n"
        return res

    def accept(self, visitor):
        return visitor.visitStmWhile(self)


class Break(Stm):
    def __init__(self):
        super().__init__(BREAK)

    def __str__(self, i=0):
        return " " * i + "[ break ]\n"

    def accept(self, visitor):
        return visitor.visitStmBreak(self)


class Return(Stm):
    def __init__(self, ret_exp: E.Exp):
        super().__init__(RETURN)
        self.ret_exp = ret_exp

    def __str__(self, i=0):
        return " " * i + "[ return " + str(self.ret_exp) + "]\n"

    def accept(self, visitor):
        return visitor.visitStmReturn(self)


class Print(Stm):
    def __init__(self, print_exp: E.Exp):
        super().__init__(PRINT)
        self.print_exp = print_exp

    def __str__(self, i=0):
        return " " * i + "[ print " + str(self.print_exp) + "]\n"

    def accept(self, visitor):
        return visitor.visitStmPrint(self)


class Exp(Stm):
    def __init__(self, exp: E.Exp):
        super().__init__(EXP_STM)
        self.exp = exp

    def __str__(self, i=0):
        return " " * i + "[ exp " + str(self.exp) + "]\n"

    def accept(self, visitor):
        return visitor.visitStmExp(self)
