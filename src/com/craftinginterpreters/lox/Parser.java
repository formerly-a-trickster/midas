package com.craftinginterpreters.lox;

import java.util.ArrayList;
import java.util.List;

import static com.craftinginterpreters.lox.TokenType.*;

class Parser {
	private static class ParseError extends RuntimeException {}

	private final List<Token> tokens;
	private int current = 0;

	Parser(List<Token> tokens) {
		this.tokens = tokens;
	}

	// varDecl -> "var" IDENTIFIER ( "=" expression )? ";"

	List<Stmt> parse() {
		// program -> statement* EOF
		List<Stmt> statements = new ArrayList<>();
		while (!isAtEnd()) {
			statements.add(declaration());
		}

		return statements;
	}

	private Stmt declaration() {
		// declaration -> varDecl
		//              | statement
		try {
			if (match(VAR)) return varDeclaration();

			return statement();
		}
		catch (ParseError error) {
			synchronize();
			return null;
		}
	}

	private Stmt varDeclaration() {
		// varDecl -> "var" IDENTIFIER ( "=" expression )? ";"
		Token name = consume(IDENTIFIER, "Expected variable name.");

		Expr initializer = null;
		if (match(EQUAL)) {
			initializer = expression();
		}

		consume(SEMICOLON, "Expected ';' after variable declaration.");
		return new Stmt.Var(name, initializer);
	}

	private Stmt statement() {
		// statement -> exprStmt
		//            | ifStmt
		//            | whileStmt
		//            | printStmt
		//            | block
		if (match(IF)) return ifStatement();
		if (match(PRINT)) return printStatement();
		if (match(WHILE)) return whileStatement();
		if (match(LEFT_BRACE)) return new Stmt.Block(block());
		else return expressionStatement();
	}

	private Stmt ifStatement() {
		// ifStmt -> "if" "(" condition ")" statement ( "else" statement )?
		consume(LEFT_PAREN, "Expected '(' after 'if'.");
		Expr condition = expression();
		consume(RIGHT_PAREN, "Expected ')' after if condition.");

		Stmt thenBranch = statement();
		Stmt elseBranch = null;
		if (match(ELSE)) {
			elseBranch = statement();
		}

		return new Stmt.If(condition, thenBranch, elseBranch);
	}

	private Stmt whileStatement() {
		// whileStmt -> "while" "(" condition ")" statement
		consume(LEFT_PAREN, "Expected '(' after 'while'.");
		Expr condition = expression();
		consume(RIGHT_PAREN, "Expected ')' after while condition.");
		Stmt body = statement();

		return new Stmt.While(condition, body);

	}

	private Stmt printStatement() {
		// printStmt -> "print" expression ";"
		Expr value = expression();
		consume(SEMICOLON, "Expected ';' after print value.");
		return new Stmt.Print(value);
	}

	private Stmt expressionStatement() {
		// exprStmt -> expression ";"
		Expr expr = expression();
		consume(SEMICOLON, "Expected ';' after expression.");
		return new Stmt.Expression(expr);
	}

	private List<Stmt> block() {
		// block -> "{" declaration "}"
		List<Stmt> statements = new ArrayList<>();

		while (!check(RIGHT_BRACE) && !isAtEnd()) {
			statements.add(declaration());
		}

		consume(RIGHT_BRACE, "Expected '}' after block.");
		return statements;
	}

	private Expr expression() {
		// expression -> commaExpression
		return commaExpression();
	}

	private Expr commaExpression() {
		// commaExpression -> assignment ( "," assignment )*
		Expr leftmost = assignment();

		while (match(COMMA)) {
			Token operator = previous();
			Expr right = assignment();
			leftmost = new Expr.Binary(leftmost, operator, right);
		}

		return leftmost;
	}

	private Expr assignment() {
		// assignment -> ternary ( "=" assignment )*
		Expr leftmost = ternary();

		if (match(EQUAL)) {
			Token equals = previous();
			Expr value = assignment();

			if (leftmost instanceof Expr.Variable) {
				Token name = ((Expr.Variable)leftmost).name;
				return new Expr.Assign(name, value);
			}

			error(equals, "Invalid assignment target");
		}

		return leftmost;
	}

	private Expr ternary() {
		// ternary -> or ( "?" expression ":" ternary )?
		Expr leftmost = or();

		if (match(QUESTION)) {
			Expr antecedent = expression();
			consume(COLON, "Unfinished ternary operator. Expected ':'.");
			Expr precedent = ternary();
			return new Expr.Ternary(leftmost, antecedent, precedent);
		}
		return leftmost;
	}

	private Expr or() {
		// or -> and ( "or" and )*
		Expr leftmost = and();

		while (match(OR)) {
			Token operator = previous();
			Expr right = and();
			leftmost = new Expr.Logical(leftmost, operator, right);
		}

		return leftmost;
	}

	private Expr and() {
		// and -> equality ( "and" equality )*
		Expr leftmost = equality();

		while (match(AND)) {
			Token operator = previous();
			Expr right = equality();
			leftmost = new Expr.Logical(leftmost, operator, right);
		}

		return leftmost;
	}

	private Expr equality() {
		// equality -> comparison ( ( "!=" | "==" ) comparison )*
		Expr leftmost = comparison();

		while (match(BANG_EQUAL, EQUAL_EQUAL)) {
			Token operator = previous();
			Expr right = comparison();
			leftmost = new Expr.Binary(leftmost, operator, right);
		}

		return leftmost;
	}

	private Expr comparison() {
		// comparison -> addition ( ( ">" | ">=" | "<" | "<=" ) addition )*
		Expr left = addition();

		while (match(GREATER, GREATER_EQUAL, LESS, LESS_EQUAL)) {
			Token operator = previous();
			Expr right = addition();
			left = new Expr.Binary(left, operator, right);
		}

		return left;
	}

	private Expr addition() {
		// addition -> multiplication ( ( "-" | "+" ) multiplication )*
		Expr left = multiplication();

		while (match(MINUS, PLUS)) {
			Token operator = previous();
			Expr right = multiplication();
			left = new Expr.Binary(left, operator, right);
		}

		return left;
	}

	private Expr multiplication() {
		// multiplication -> unary ( ( "/" | "*" ) unary )*
		Expr left = unary();

		while (match(SLASH, STAR)) {
			Token operator = previous();
			Expr right = unary();
			left = new Expr.Binary(left, operator, right);
		}

		return left;
	}

	private Expr unary() {
		// unary -> ( "!" | "-" ) unary
		//			  | primary
		if (match(BANG, MINUS)) {
			Token operator = previous();
			Expr right = unary();
			return new Expr.Unary(operator, right);
		}

		return primary();
	}

	private Expr primary() {
		// primary -> "true" | "false" | "nil"
		//          | NUMBER | STRING
		//          | "(" expression ")"
		//          | IDENTIFIER
		if (match(FALSE)) return new Expr.Literal(false);
		if (match(TRUE)) return new Expr.Literal(true);
		if (match(NIL)) return new Expr.Literal(null);

		if (match(NUMBER, STRING)) {
			return new Expr.Literal(previous().literal);
		}

		if (match(LEFT_PAREN)) {
			Expr expr = expression();
			consume(RIGHT_PAREN, "Expected ')' after expression.");
			return new Expr.Grouping(expr);
		}

		if (match(IDENTIFIER)) {
			return new Expr.Variable(previous());
		}

		throw error(peek(), "Expected expression.");
	}

	private boolean match(TokenType... types) {
		for (TokenType type : types) {
			if (check(type)) {
				advance();
				return true;
			}
		}

		return false;
	}

	private Token consume(TokenType type, String message) {
		if (check(type)) return advance();

		throw error(peek(), message);
	}

	private boolean check(TokenType tokenType) {
		if (isAtEnd()) return false;
		return peek().type == tokenType;
	}

	private Token advance() {
		if (!isAtEnd()) current++;
		return previous();
	}

	private boolean isAtEnd() {
		return peek().type == EOF;
	}

	private Token peek() {
		return tokens.get(current);
	}

	private Token previous() {
		return tokens.get(current - 1);
	}

	private ParseError error(Token token, String message) {
		Lox.error(token, message);
		return new ParseError();
	}

	private void synchronize() {
		advance();

		while (!isAtEnd()) {
			if (previous().type == SEMICOLON) return;

			switch (peek().type) {
				case CLASS:
				case FUN:
				case VAR:
				case FOR:
				case IF:
				case WHILE:
				case PRINT:
				case RETURN:
					return;
			}

			advance();
		}
	}
}