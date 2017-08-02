package com.craftinginterpreters.lox;

import java.util.List;

import static com.craftinginterpreters.lox.TokenType.*;

class Parser {
	private static class ParseError extends RuntimeException {}

	private final List<Token> tokens;
	private int current = 0;

	Parser(List<Token> tokens) {
		this.tokens = tokens;
	}

	Expr parse() {
		try {
			return expression();
		}
		catch (ParseError error) {
			return null;
		}
	}

	private Expr expression() {
		// expression -> equality ( "?" expression ":" expression )
		//             | equality
		Expr initial = equality();

		if (match(QUESTION)) {
			Expr antecedent = equality();
			if (!match(COLON)) throw error(peek(), "Unfinished ternary operator. Expected ':'.");
			Expr precendent = equality();
			return new Expr.Ternary(initial, antecedent, precendent);
		}
		return initial;
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
			consume(RIGHT_PAREN, "Expect ')' after expression.");
			return new Expr.Grouping(expr);
		}

		throw error(peek(), "Expect expression.");
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