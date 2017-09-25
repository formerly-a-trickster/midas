from typing import List, Union
import interpreter as Int
import statement as Stm
import expression as Exp

FUNCTION = "FUNCTION"


def reverse(seq):
    ans = seq[:0]
    for i in range(len(seq)):
        ans = seq[i:i+1] + ans
    return ans


class Resolver(object):
    def __init__(self, interpreter: Int.Interpreter):
        self.interpreter = interpreter
        self.scopes = [] # List[Dict[str, bool]]

    def beginScope(self):
        self.scopes.append({})

    def endScope(self):
        self.scopes.pop()

    def declare(self, name: str):
        if len(self.scopes) == 0:
            return
        scope = self.scopes[-1]
        scope[name] = False

    def define(self, name: str):
        if len(self.scopes) == 0:
            return
        scope = self.scopes[-1]
        scope[name] = True

    # resolving helpers
    def resolveLocal(self, exp: Exp.Ident, name: str):
        for i in range(len(self.scopes) - 1, -1, -1):
            if name in self.scopes[i]:
                self.interpreter.resolve(exp, len(self.scopes) - 1 - i)
                break

    def resolveFunction(self, fun: Stm.FunDecl, type: str):
        self.beginScope()
        for formal in fun.formals:
            self.declare(formal)
            self.define(formal)
        for stm in fun.body.block:
            self.resolve(stm)
        self.endScope()

    def resolveAst(self, ast: List[Stm.Stm]):
        for stm in ast:
            self.resolve(stm)

    def resolve(self, ent: Union[Exp.Exp, Stm.Stm]):
        ent.accept(self)

    # statement resolving
    def visitStmVarDecl(self, stm: Stm.VarDecl):
        self.declare(stm.name)
        self.resolve(stm.exp)
        self.define(stm.name)

    def visitStmFunDecl(self, stm: Stm.FunDecl):
        self.declare(stm.name)
        self.define(stm.name)

        self.resolveFunction(stm, FUNCTION)

    def visitStmBlock(self, stm: Stm.Block):
        self.beginScope()
        for stm in stm.block:
            self.resolve(stm)
        self.endScope()

    # uninteresting statements
    def visitStmIf(self, stm: Stm.If):
        self.resolve(stm.cond)
        self.resolve(stm.then_block)
        if stm.else_block is not None:
            self.resolve(stm.else_block)

    def visitStmWhile(self, stm: Stm.While):
        self.resolve(stm.cond)
        self.resolve(stm.body)

    def visitStmBreak(self, stm: Stm.Break):
        pass

    def visitStmReturn(self, stm: Stm.Return):
        self.resolve(stm.ret_exp)

    def visitStmPrint(self, stm: Stm.Print):
        self.resolve(stm.print_exp)

    def visitStmExp(self, stm: Stm.Exp):
        self.resolve(stm.exp)

    # expression resolving
    def visitExpAssign(self, exp: Exp.Assign):
        self.resolve(exp.exp)
        self.resolveLocal(exp, exp.name)

    def visitExpIdent(self, exp: Exp.Ident):
        if not len(self.scopes) == 0 and \
                exp.name in self.scopes[-1] and \
                self.scopes[-1][exp.name] == False:
            raise Exception("Cannot read variable %s in its own " +
                            "initializer." % exp.name)
        self.resolveLocal(exp, exp.name)

    # uninteresting expressions
    def visitExpBinary(self, exp: Exp.Binary):
        self.resolve(exp.left)
        self.resolve(exp.right)

    def visitExpUnary(self, exp: Exp.Unary):
        self.resolve(exp.exp)

    def visitExpCall(self, exp: Exp.Call):
        for param in exp.params:
            self.resolve(param)
        self.resolve(exp.callee)

    def visitExpLiteral(self, exp: Exp.Literal):
        pass
