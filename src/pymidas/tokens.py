EOF = "TOK_EOF"

BANG = "TOK_BANG"
COMMA = "TOK_COMMA"
EQUAL = "TOK_EQUAL"
GREAT = "TOK_GREAT"
LESS = "TOK_LESS"
MINUS = "TOK_MINUS"
PAREN_LEFT = "TOK_PAREN_LEFT"
PAREN_RIGHT = "TOK_PAREN_RIGHT"
PERCENT = "TOK_PERCENT"
PLUS = "TOK_PLUS"
SEMICOLON = "TOK_SEMICOLON"
SLASH = "TOK_SLASH"
STAR = "TOK_STAR"

BANG_EQUAL = "TOK_BANG_EQUAL"
EQUAL_EQUAL = "TOK_EQUAL_EQUAL"
GREAT_EQUAL = "TOK_GREAT_EQUAL"
LESS_EQUAL = "TOK_LESS_EQUAL"
PLUS_PLUS = "TOK_PLUS_PLUS"
SLASH_SLASH = "TOK_SLASH_SLASH"

DOUBLE = "TOK_DOUBLE"
FALSE = "TOK_FALSE"
INTEGER = "TOK_INTEGER"
NIL = "TOK_NIL"
STRING = "TOK_STRING"
TRUE = "TOK_TRUE"

AND = "TOK_AND"
BREAK = "TOK_BREAK"
DO = "TOK_DO"
ELSE = "TOK_ELSE"
END = "TOK_END"
FOR = "TOK_FOR"
FUN = "TOK_FUN"
IDENTIFIER = "TOK_IDENTIFIER"
IF = "TOK_IF"
OR = "TOK_OR"
PRINT = "TOK_PRINT"
RETURN = "TOK_RETURN"
VAR = "TOK_VAR"
WHILE = "TOK_WHILE"

to_str = {
    "TOK_EOF": "EOF",

    "TOK_BANG"       : "!",
    "TOK_COMMA"      : ",",
    "TOK_EQUAL"      : "=",
    "TOK_GREAT"      : ">",
    "TOK_LESS"       : "<",
    "TOK_MINUS"      : "-",
    "TOK_PAREN_LEFT" : "(",
    "TOK_PAREN_RIGHT": ")",
    "TOK_PERCENT"    : "%",
    "TOK_PLUS"       : "+",
    "TOK_SEMICOLON"  : ";",
    "TOK_SLASH"      : "/",
    "TOK_STAR"       : "*",

    "TOK_BANG_EQUAL" : "!=",
    "TOK_EQUAL_EQUAL": "==",
    "TOK_GREAT_EQUAL": ">=",
    "TOK_LESS_EQUAL" : "<=",
    "TOK_PLUS_PLUS"  : "++",
    "TOK_SLASH_SLASH": "//",

    "TOK_DOUBLE" : "double",
    "TOK_FALSE"  : "false",
    "TOK_INTEGER": "integer",
    "TOK_NIL"    : "nil",
    "TOK_STRING" : "string",
    "TOK_TRUE"   : "true",

    "TOK_AND"   : "and",
    "TOK_BREAK" : "break",
    "TOK_DO"    : "do",
    "TOK_ELSE"  : "else",
    "TOK_END"   : "end",
    "TOK_FOR"   : "for",
    "TOK_FUN"   : "fun",
    "TOK_IDENTIFIER": "ID",
    "TOK_IF"    : "if",
    "TOK_OR"    : "or",
    "TOK_PRINT" : "print",
    "TOK_RETURN": "return",
    "TOK_VAR"   : "var",
    "TOK_WHILE" : "while"
}
