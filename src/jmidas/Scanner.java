package jmidas;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import static jmidas.TokenType.*;

class Scanner {
	private final String source;
	private final List<Token> tokens = new ArrayList<>();
	private int start = 0;
	private int current = 0;
	private int line = 1;
	private static final Map<String, TokenType> keywords;

	static {
		keywords = new HashMap<>();
		keywords.put("and", AND);
		keywords.put("break", BREAK);
		keywords.put("class", CLASS);
		keywords.put("do", DO);
		keywords.put("end", END);
		keywords.put("else", ELSE);
		keywords.put("true", TRUE);
		keywords.put("false", FALSE);
		keywords.put("for", FOR);
		keywords.put("fun", FUN);
		keywords.put("if", IF);
		keywords.put("nil", NIL);
		keywords.put("or", OR);
		keywords.put("print", PRINT);
		keywords.put("return", RETURN);
		keywords.put("super", SUPER);
		keywords.put("this", THIS);
		keywords.put("var", VAR);
		keywords.put("while", WHILE);
	}

	Scanner(String source) {
		this.source = source;
	}

	List<Token> scanTokens() {
		while (!isAtEnd()) {
			start = current;
			scanToken();
		}

		tokens.add(new Token(EOF, "", null, line));
		return tokens;
	}

	private void scanToken() {
		char c = advance();
		switch (c) {
			case '(': addToken(LEFT_PAREN); break;
			case ')': addToken(RIGHT_PAREN); break;
			case ',': addToken(COMMA); break;
			case ':': addToken(COLON); break;
			case '.': addToken(DOT); break;
			case '-': addToken(MINUS); break;
			case '%': addToken(PERCENT); break;
			case '?': addToken(QUESTION); break;
			case ';': addToken(SEMICOLON); break;
			case '/': addToken(SLASH); break;
			case '*': addToken(STAR); break;
			case '+': addToken(match('+') ? PLUS_PLUS : PLUS); break;
			case '!': addToken(match('=') ? BANG_EQUAL : BANG); break;
			case '=': addToken(match('=') ? EQUAL_EQUAL : EQUAL); break;
			case '<': addToken(match('=') ? LESS_EQUAL : LESS); break;
			case '>': addToken(match('=') ? GREATER_EQUAL : GREATER); break;
			//case '/': addToken(match('/') ? SLASH_SLASH : SLASH); break;
			case '#':
				// A comment goes until the end of the line
				while (peek() != '\n' && !isAtEnd()) advance();
				break;

			case ' ':
			case '\r':
			case '\t':
				// Ignore whitespace.
				break;

			case '\n':
				line++;
				break;

			case '"': string(); break;

			default:
				if (isDigit(c)) {
					number();
				}
				else if (isAlpha(c)) {
					identifier();
				}
				else {
					Midas.error(line, "Unexpected character.");
				}
				break;
		}
	}

	private void skipBlockComment() {
		int nesting = 1;
		while (nesting > 0) {
			if (peek() == '\0') {
				Midas.error(line, "Unterminated block comment.");
				return;
			}

			if (peek() == '/' && peekNext() == '*') {
				advance();
				advance();
				nesting++;
				continue;
			}

			if (peek() == '*' && peekNext() == '/') {
				advance();
				advance();
				nesting--;
				continue;
			}

			if (peek() == '\n') {
				line++;
			}

			// Regular character.
			advance();
		}
	}

	private void identifier() {
		while (isAlphaNumeric(peek())) advance();

		// See if identifier is a reserved word.
		String text = source.substring(start, current);

		TokenType type = keywords.get(text);
		if (type == null) type = IDENTIFIER;
		addToken(type);
	}

	private void number() {
		while (isDigit(peek())) advance();

		// Look for a fractional part.
		if (peek() == '.' && isDigit(peekNext())) {
			// Consume the "."
			advance();

			while (isDigit(peek())) advance();
		}

		addToken(NUMBER,
			Double.parseDouble(source.substring(start, current)));
	}

	private void string() {
		while (peek() != '"' && !isAtEnd()) {
			if (peek() == '\n') line++;
			advance();
		}

		// Unterminated string.
		if (isAtEnd()) {
			Midas.error(line, "Unterminated string.");
			return;
		}

		// The closing ".
		advance();

		// Trim the aurrounding quotes.
		String value = source.substring(start + 1, current - 1);
		addToken(STRING, value);
	}

	private boolean match(char expected) {
		if (isAtEnd()) return false;
		if (source.charAt(current) != expected) return false;

		current++;
		return true;
	}

	private char peek() {
		if (isAtEnd())  return '\0';
		return source.charAt(current);
	}

	private char peekNext() {
		if (current + 1 >= source.length()) return '\0';
		return source.charAt(current + 1);
	}

	private boolean isAlpha(char c) {
		return (c >= 'a' && c <= 'z') ||
					 (c >= 'A' && c <= 'Z') ||
						c == '_';
	}

	private boolean isAlphaNumeric(char c) {
		return isAlpha(c) || isDigit(c);
	}

	private boolean isDigit(char c) {
		return c >= '0' && c <= '9';
	}

	private boolean isAtEnd() {
		return current >= source.length();
	}

	private char advance() {
		current++;
		return source.charAt(current - 1);
	}

	private void addToken(TokenType type) {
		addToken(type, null);
	}

	private void addToken(TokenType type, Object literal) {
		String text = source.substring(start, current);
		tokens.add(new Token(type, text, literal, line));
	}
}
