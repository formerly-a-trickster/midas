from lexer import Token, Lexer
from toks import *
from stmt import *
from expr import *


class ParserError(Exception):
    pass


class Parser():
    def __init__(self):
        self.lex = Lexer()
        self.path = None
        self.prev_tok = None
        self.this_tok = None
        self.loop_depth = 0
        self.fun_depth = 0

    def parse(self, path):
        with open(path, "r") as source:
            text = source.read() + "\0"
        self.lex.feed(text)
        self.tok_next()
        return self.program()

    def program(self):
        # program -> statement* "EOF"
        stmts = []
        while not self.tok_is(TOK_EOF):
            stm = self.declaration()
            stmts.append(stm)
        return stmts

    def declaration(self):
        # declaration -> var_decl ";"
        #              | fun_decl ";"
        #              | statement
        if self.tok_matches(TOK_VAR):
            stm = self.var_decl()
        elif self.tok_matches(TOK_FUN):
            stm = self.fun_decl()
        else:
            stm = self.statement()
        return stm

    def var_decl(self):
        # var_decl -> ^var^ identifier "=" expression ";"
        self.tok_consume(TOK_IDENTIFIER,
                "A variable should follow the `var` keyword")
        name = self.prev_tok.lexeme
        self.tok_consume(TOK_EQUAL,
                "An equal sign should follow the variable's name")
        value = self.expression()
        self.tok_consume(TOK_SEMICOLON,
                "Missing semicolon after variable declaration")
        return StmVarDecl(name, value)

    def fun_decl(self):
        # fun_decl -> ^fun^ identifier "(" formals? ")" block
        # formals  -> identifier ( "," identifier )*
        self.tok_consume(TOK_IDENTIFIER,
                "A function name should follow the `fun` keuyword")
        name = self.prev_tok.lexeme
        self.tok_consume(TOK_PAREN_LEFT,
                "Expected an opening paren after the function name")
        formals = []

        if not self.tok_matches(TOK_PAREN_RIGHT):
            self.tok_consume(TOK_IDENTIFIER,
                    "Function parameters should be a comma-separated list" +
                    "of identifiers")
            formals.append(self.prev_tok.lexeme)

            while self.tok_matches(TOK_COMMA):
                self.tok_consume(TOK_IDENTIFIER,
                        "Function parameters should be a comma-separated " +
                        "list of identifiers")
                formals.append(self.prev_tok.lexeme)
            self.tok_consume(TOK_PAREN_RIGHT,
                    "Expected a closing paren after function arguments")
        self.tok_consume(TOK_DO,
                "Expected a do...end block after function parameter list")
        self.fun_depth += 1
        body = self.block()
        self.fun_depth -= 1

        return StmFunDecl(name, formals, body)

    def statement(self):
        # statement -> block
        #            | if_stm
        #            | while_stm
        #            | for_stm
        #            | break_stm
        #            | return_stm
        #            | print_stm
        #            | exp_stm
        if self.tok_matches(TOK_DO):
            stm = self.block()
        elif self.tok_matches(TOK_IF):
            stm = self.if_cond()
        elif self.tok_matches(TOK_WHILE):
            stm = self.while_cond()
        elif self.tok_matches(TOK_FOR):
            stm = self.for_cond()
        elif self.tok_matches(TOK_BREAK):
            stm = self.break_stm()
        elif self.tok_matches(TOK_RETURN):
            stm = self.return_stm()
        elif self.tok_matches(TOK_PRINT):
            stm = self.print_stm()
        else:
            stm = self.exp_stm()

        return stm

    def block(self):
        # block -> ^do^ declaration* "end"
        stmts = []
        while not self.tok_is(TOK_END) and not self.tok_is(TOK_EOF):
            stm = self.declaration()
            stmts.append(stm)
        self.tok_consume(TOK_END,
                "Missing `end` keyword after block")
        return StmBlock(stmts)

    def if_cond(self):
        # if_stm -> ^if^ "(" expression ")" statement ( "else" statement )?
        self.tok_consume(TOK_PAREN_LEFT,
                "Expected an opening paren after `if` keyword")
        cond = self.expression()
        self.tok_consume(TOK_PAREN_RIGHT,
                "Expected a closing paren after the if condition")
        then_block = self.statement()

        if self.tok_matches(TOK_ELSE):
            else_block = self.statement()
        else:
            else_block = None
        return StmIf(cond, then_block, else_block)

    def while_cond(self):
        # while_stm -> ^while^ "(" expresion ")" statement
        self.tok_consume(TOK_PAREN_LEFT,
                "Expected an opening paren after `while` keyword")
        cond = self.expression()
        self.tok_consume(TOK_PAREN_RIGHT,
                "Expected a closing paren after the while condition")
        self.loop_depth += 1
        body = self.statement()
        self.loop_depth -= 1

        return StmWhile(cond, body)

    def for_cond(self):
        # for_stm -> ^for^ "("
        #                      ( var_decl | exp_stm | ";" )
        #                      expression? ";"
        #                      assignment?
        #                  ")" statement
        # NB: var_decl and exp_stm already contain a semicolon
        self.tok_consume(TOK_PAREN_LEFT,
                "Expected an opening paren after the `for` keyword")
        if self.tok_matches(TOK_VAR):
            init = self.var_decl()
        elif self.tok_matches(TOK_SEMICOLON):
            init = None
        else:
            init = self.exp_stm()

        if not self.tok_matches(TOK_SEMICOLON):
            cond = self.expression()
            self.tok_consume(TOK_SEMICOLON,
                    "Expected a semicolon after the for condition")
        else:
            cond = None

        if not self.tok_matches(TOK_PAREN_RIGHT):
            incr = self.assignment()
            self.tok_consume(TOK_PAREN_RIGHT,
                    "Expected a closing paren after the for construct")
        else:
            incr = None

        self.loop_depth += 1
        body = self.statement()
        self.loop_depth -= 1

        if incr != None:
            incr_stm = StmExp(incr)
            if body.kind == STM_BLOCK:
                body.block.append(incr_stm)
            else:
                stmts = []
                stmts.append(body)
                stmts.append(incr_stm)
                body = StmBlock(stmts)

        if cond == None:
            tok = Token("true", TOK_TRUE, 4, 0, 0)
            cond = ExpLiteral(tok)

        loop = StmWhile(cond, body)

        if init != None:
            stmts = []
            stmts.append(init)
            stmts.append(loop)
            loop = StmBlock(stmts)

        return loop

    def break_stm(self):
        # ^break^ ";"
        if self.loop_depth != 0:
            self.tok_consume(TOK_SEMICOLON,
                    "Missing semicolon after break statement")
        else:
            raise ParserError("Encountered break statement outside of a for " +
                              "or while loop")
        return StmBreak()

    def return_stm(self):
        # return_stm -> ^return^ expression? ";"
        if self.fun_depth != 0:
            if not self.tok_matches(TOK_SEMICOLON):
                ret_exp = self.expression()
                self.tok_consume(TOK_SEMICOLON,
                        "Missing semicolon after return statement")
                return StmReturn(ret_exp)
            else:
                nil = Token("nil", TOK_NIL, 3, 0, 0)
                return StmReturn(ExpLiteral(nil))
        else:
            raise ParserError("Encountered a return statements outside of a " +
                    "function declaration")

    def print_stm(self):
        # print_stm -> ^print^ expression ";"
        exp = self.expression()
        self.tok_consume(TOK_SEMICOLON, "Missing semicolon after print statement")
        return StmPrint(exp)

    def exp_stm(self):
        # exp_stm -> expression ";"
        exp = self.expression()
        self.tok_consume(TOK_SEMICOLON,
                "Missing semicolon after expression statement")
        return StmExp(exp)

    def expression(self):
        # expression -> assigment
        return self.assignment()

    def assignment(self):
        # assignment -> concat ( "=" assignment )
        left = self.concat()
        if self.tok_matches(TOK_EQUAL):
            right = self.assignment()
            if left.kind == EXP_IDENT:
                return ExpAssign(left.name, right)
            else:
                raise ParserError("Cannot assign to this target")
        return left

    def concat(self):
        # concat -> or ( "++" or )*
        left = self.logic_or()
        while self.tok_matches(TOK_PLUS_PLUS):
            op = self.prev_tok.kind
            right = self.logic_or()
            left = ExpBinary(op, left, right)
        return left

    def logic_or(self):
        # or -> and ( "or" and )*
        left = self.logic_and()
        while self.tok_matches(TOK_OR):
            op = self.prev_tok.kind
            right = self.logic_and()
            left = ExpBinary(op, left, right)
        return left

    def logic_and(self):
        # and -> equality ( "or" equality )*
        left = self.equality()
        while self.tok_matches(TOK_AND):
            op =  self.prev_tok.kind
            right = self.equality()
            left = ExpBinary(op, left, right)
        return left

    def equality(self):
        # equality -> ordering ( ( "!=" | "==" ) ordering )*
        left = self.ordering()
        while self.tok_matches(TOK_BANG_EQUAL) or \
                self.tok_matches(TOK_EQUAL_EQUAL):
            op = self.prev_tok.kind
            right = self.ordering()
            left = ExpBinary(op, left, right)
        return left

    def ordering(self):
        # ordering -> addition ( ( ">" | ">=" | "<" | "<=" ) addition )*
        left = self.addition()
        while self.tok_matches(TOK_GREAT) or \
                self.tok_matches(TOK_GREAT_EQUAL) or \
                self.tok_matches(TOK_LESS) or \
                self.tok_matches(TOK_LESS_EQUAL):
            op = self.prev_tok.kind
            right = self.addition()
            left = ExpBinary(op, left, right)
        return left

    def addition(self):
        # addition -> multiplication ( ( "-" | "+" ) multiplication )*
        left = self.multiplication()
        while self.tok_matches(TOK_MINUS) or \
                self.tok_matches(TOK_PLUS):
            op = self.prev_tok.kind
            right = self.multiplication()
            left = ExpBinary(op, left, right)
        return left

    def multiplication(self):
        # multiplication -> unary ( ( "/" | "//" | "*" | "%" ) unary )*
        left = self.unary()
        while self.tok_matches(TOK_SLASH) or \
                self.tok_matches(TOK_SLASH_SLASH) or \
                self.tok_matches(TOK_STAR) or \
                self.tok_matches(TOK_PERCENT):
            op = self.prev_tok.kind
            right = self.unary()
            left = ExpBinary(op, left, right)
        return left

    def unary(self):
        # unary -> ( ( "!" | "-" ) unary )
        #        | call
        if self.tok_matches(TOK_BANG) or self.tok_matches(TOK_MINUS):
            op = self.prev_tok.kind
            exp = self.unary()
            return ExpUnary(op, exp)
        else:
            return self.call()

    def call(self):
        # call -> primary ( "(" parameters? ")" )*
        callee = self.primary()
        while self.tok_matches(TOK_PAREN_LEFT):
            params = []
            if not self.tok_matches(TOK_PAREN_RIGHT):
                exp = self.expression()
                params.append(exp)
                while self.tok_matches(TOK_COMMA):
                    exp = self.expression()
                    params.append(exp)
                self.tok_consume(TOK_PAREN_RIGHT,
                        "Expected a closing paren after arguments")
            callee = ExpCall(callee, params)
        return callee

    def primary(self):
        # primary -> IDENTIFIER
        #          | INTEGER | DOUBLE | STRING | NIL | "false" | "true" |
        #          | "(" expression ")"
        if self.tok_matches(TOK_IDENTIFIER):
            exp = ExpIdent(self.prev_tok.lexeme)
        elif self.tok_matches(TOK_INTEGER) or \
                self.tok_matches(TOK_DOUBLE) or \
                self.tok_matches(TOK_STRING) or \
                self.tok_matches(TOK_NIL) or \
                self.tok_matches(TOK_FALSE) or \
                self.tok_matches(TOK_TRUE):
            exp = ExpLiteral(self.prev_tok)
        elif self.tok_matches(TOK_PAREN_LEFT):
            exp = self.expression()
            self.tok_consume(TOK_PAREN_RIGHT, "Expected a closing paren")
        else:
            raise ParserError("Expected number, paren or keyword. Got " +
                    str(self.this_tok))
        return exp

    def tok_next(self):
        self.prev_tok = self.this_tok
        self.this_tok = self.lex.get_tok()
        return self.prev_tok

    def tok_matches(self, kind):
        if self.this_tok.kind == kind:
            self.tok_next()
            return True
        else:
            return False

    def tok_consume(self, kind, message):
        if self.this_tok.kind == kind:
            self.tok_next()
        else:
            raise ParserError(message)

    def tok_is(self, kind):
        return self.this_tok.kind == kind

    def tok_was(self, kind):
        return self.prev_tok.kind == kind
