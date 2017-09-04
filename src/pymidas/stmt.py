from typing import List
from expr import Exp

STM_VAR_DECL = "STM_VAR_DECL"
STM_FUN_DECL = "STM_FUN_DECL"
STM_BLOCK    = "STM_BLOCK"
STM_IF       = "STM_IF"
STM_WHILE    = "STM_WHILE"
STM_BREAK    = "STM_BREAK"
STM_RETURN   = "STM_RETURN"
STM_PRINT    = "STM_PRINT"
STM_EXP_STM  = "STM_EXP_STM"


class Stm(object):
    def __init__(self, kind):
        self.kind = kind


class StmVarDecl(Stm):
    def __init__(self, name: str, exp: Exp):
        super().__init__(STM_VAR_DECL)
        self.name = name
        self.exp = exp

    def __str__(self, i=0):
        return " " * i + "[ var " + self.name + " as " + str(self.exp) + "]\n"


class StmFunDecl(Stm):
    def __init__(self, name: str, formals: List[str], body: Stm):
        super().__init__(STM_FUN_DECL)
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


class StmBlock(Stm):
    def __init__(self, block: List[Stm]):
        super().__init__(STM_BLOCK)
        self.block = block

    def __str__(self, i=0):
        res = " " * i + "[\n"
        for stm in self.block:
            res += stm.__str__(i + 4)
        res += " " * i + "]\n"
        return res


class StmIf(Stm):
    def __init__(self, cond: Exp, then_block: Stm, else_block: Stm):
        super().__init__(STM_IF)
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


class StmWhile(Stm):
    def __init__(self, cond: Exp, body: Stm):
        super().__init__(STM_WHILE)
        self.cond = cond
        self.body = body

    def __str__(self, i=0):
        res = " " * i + "[ while " + str(self.cond) + "\n"
        res += " " * (i + 2) + "loop\n"
        res += self.body.__str__(i + 4)
        res += " " * i + "]\n"
        return res


class StmBreak(Stm):
    def __init__(self):
        super().__init__(STM_BREAK)

    def __str__(self, i=0):
        return " " * i + "[ break ]\n"


class StmReturn(Stm):
    def __init__(self, ret_exp: Exp):
        super().__init__(STM_RETURN)
        self.ret_exp = ret_exp

    def __str__(self, i=0):
        return " " * i + "[ return " + str(self.ret_exp) + "]\n"


class StmPrint(Stm):
    def __init__(self, print_exp: Exp):
        super().__init__(STM_PRINT)
        self.print_exp = print_exp

    def __str__(self, i=0):
        return " " * i + "[ print " + str(self.print_exp) + "]\n"


class StmExp(Stm):
    def __init__(self, exp: Exp):
        super().__init__(STM_EXP_STM)
        self.exp = exp

    def __str__(self, i=0):
        return " " * i + "[ exp " + str(self.exp) + "]\n"
