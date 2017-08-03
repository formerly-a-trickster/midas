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

	List<Stmt> parse() {
		// program -> statement* EOF
		List<Stmt> statements = new ArrayList<>();
		while (!isAtEnd()) {
			statements.add(statement());
		}

		return statements;
	}

	private Stmt statement() {
		// statement -> exprStmt
		//            | printStmt
		if (match(PRINT)) return printStatement();
		else return expressionStatement();
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

	private Expr expression() {
		// expression -> commaExpression
		return commaExpression();
	}

	private Expr commaExpression() {
		// commaExpression -> ternary ( "," ternary )*
		Expr left = ternary();

		while (match(COMMA)) {
			Token operator = previous();
			Expr right = ternary();
			left = new Expr.Binary(left, operator, right);
		}

		return left;
	}

	private Expr ternary() {
		// ternary -> equality ( "?" expression ":" ternary )
		//          | equality
		Expr leftmost = equality();

		if (match(QUESTION)) {
			Expr antecedent = expression();
			consume(COLON, "Unfinished ternary operator. Expected ':'.");
			Expr precedent = ternary();
			return new Expr.Ternary(leftmost, antecedent, precedent);
		}
		return leftmost;
	}

	private Expr equality() {
		// equality -> comparison ( ( "!=" | "==" ) comparison )*
		Expr left = comparison();

		while (match(BANG_EQUAL, EQUAL_EQUAL)) {
			Token operator = previous();
			Expr right = comparison();
			left = new Expr.Binary(left, operator, right);
		}

		return left;
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
		// primary -> NUMBER | STRING | "false" | "true" | "nil"
		//          | "(" expression ")"
		if (match(NUMBER, STRING)) {
			return new Expr.Literal(previous().literal);
		}

		if (match(FALSE)) return new Expr.Literal(false);
		if (match(TRUE)) return new Expr.Literal(true);
		if (match(NIL)) return new Expr.Literal(null);

		if (match(LEFT_PAREN)) {
			Expr expr = expression();
			consume(RIGHT_PAREN, "Expected ')' after expression.");
			return new Expr.Grouping(expr);
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