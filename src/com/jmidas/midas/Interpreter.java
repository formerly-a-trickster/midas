package com.jmidas.midas;

import java.util.ArrayList;
import java.util.List;

class Interpreter implements Expr.Visitor<Object>, Stmt.Visitor<Object> {
	private static class BreakException extends RuntimeException {}

	final Environment globals = new Environment();
	private Environment environment = globals;

	Interpreter() {
		globals.define("clock", new MidasCallable() {
			@Override
			public int arity() {
				return 0;
			}

			@Override
			public Object call(Interpreter interpreter,
												 List<Object> arguments) {
				return (double)System.currentTimeMillis() / 1000.0;
			}
		});
	}

	void interpret(List<Stmt> statements) {
		try {
			for (Stmt statement : statements) {
				execute(statement);
			}
		}
		catch (RuntimeError error) {
			Midas.runtimeError(error);
		}
	}

	@Override
	public Object visitLiteralExpr(Expr.Literal expr) {
		return expr.value;
	}

	@Override
	public Object visitLogicalExpr(Expr.Logical expr) {
		Object left = evaluate(expr.left);

		if (expr.operator.type == TokenType.OR) {
			if (isTruthy(left)) return left;
		}
		else {
			if (!isTruthy(left)) return left;
		}

		return evaluate(expr.right);
	}

	@Override
	public Object visitGroupingExpr(Expr.Grouping expr) {
		return evaluate(expr.expression);
	}

	@Override
	public Object visitUnaryExpr(Expr.Unary expr) {
		Object right = evaluate(expr.right);

		switch (expr.operator.type) {
			case BANG:
				return !isTruthy(right);
			case MINUS:
				checkNumberOperand(expr.operator, right);
				return -(double)right;
		}

		// Unreachable.
		return null;
	}

	@Override
	public Object visitVariableExpr(Expr.Variable expr) {
		return environment.get(expr.name);
	}

	private void checkNumberOperand(Token operator, Object operand) {
		if (operand instanceof Double) return;
		throw new RuntimeError(operator, "Operand must be a number.");
	}

	private void checkNumberOperands(
					Token operator, Object left, Object right) {
		if (left instanceof Double && right instanceof Double) return;

		throw new RuntimeError(operator, "Operands must be a number.");
	}

	private boolean isTruthy(Object object) {
		if (object == null) return false;
		if (object instanceof Boolean) return (boolean)object;
		return true;
	}

	private boolean isEqual(Object a, Object b) {
		// nil is only equal to nil.
		if ((a == null) && (b == null)) return true;
		if (a == null) return false;

		return a.equals(b);
	}

	private String stringify(Object object) {
		if (object == null) return "nil";

		// Hack. Work around Java adding ".0" to integer-valued doubles.
		if (object instanceof Double) {
			String text = object.toString();
			if (text.endsWith(".0")) {
				text = text.substring(0, text.length() - 2);
			}

			return text;
		}

		return object.toString();
	}

	@Override
	public Object visitBinaryExpr(Expr.Binary expr) {
		Object left = evaluate(expr.left);
		Object right = evaluate(expr.right);

		switch (expr.operator.type) {
			case BANG_EQUAL: return !isEqual(left, right);
			case EQUAL_EQUAL: return isEqual(left, right);
			case GREATER:
				if (left instanceof Double && right instanceof Double) {
					return (double)left > (double)right;
				}

				if (left instanceof String && right instanceof String) {
					return ((String)left).compareTo((String)right) == 1;
				}

				throw new RuntimeError(expr.operator,
								"Operands must be two numbers or two strings");
			case GREATER_EQUAL:
				if (left instanceof Double && right instanceof Double) {
					return (double)left >= (double)right;
				}

				if (left instanceof String && right instanceof String) {
					return ((String)left).compareTo((String)right) >= 0;
				}

				throw new RuntimeError(expr.operator,
								"Operands must be two numbers or two strings");
			case LESS:
				if (left instanceof Double && right instanceof Double) {
					return (double)left < (double)right;
				}

				if (left instanceof String && right instanceof String) {
					return ((String)left).compareTo((String)right) == -1;
				}

				throw new RuntimeError(expr.operator,
								"Operands must be two numbers or two strings");
			case LESS_EQUAL:
				if (left instanceof Double && right instanceof Double) {
					return (double)left <= (double)right;
				}

				if (left instanceof String && right instanceof String) {
					return ((String)left).compareTo((String)right) <= 0;
				}

				throw new RuntimeError(expr.operator,
								"Operands must be two numbers or two strings");
			case PLUS:
				checkNumberOperands(expr.operator, left, right);
				return (double)left + (double)right;
			case PLUS_PLUS:
				return stringify(left) + stringify(right);
			case MINUS:
				checkNumberOperands(expr.operator, left, right);
				return (double)left - (double)right;
			case SLASH:
				checkNumberOperands(expr.operator, left, right);
				if ((double)right == 0)
					throw new RuntimeError(expr.operator,
									"Cannot divide by zero.");
				return (double)left / (double)right;
			case STAR:
				checkNumberOperands(expr.operator, left, right);
				return (double)left * (double)right;
			case PERCENT:
				checkNumberOperands(expr.operator, left, right);
				return (double)left % (double)right;
			case COMMA:
				return right;
		}

		// Unreachable.
		return null;
	}

	@Override
	public Object visitCallExpr(Expr.Call expr) {
		Object callee = evaluate(expr.callee);

		List<Object> arguments = new ArrayList<>();
		for (Expr argument : expr.arguments) {
			arguments.add(evaluate(argument));
		}

		if (!(callee instanceof MidasCallable)) {
			throw new RuntimeError(expr.paren,
							"Can only call functions.");
		}

		MidasCallable function = (MidasCallable)callee;
		if (arguments.size() != function.arity()) {
			throw new RuntimeError(expr.paren, "Expected " +
							function.arity() + " arguments but got " +
							arguments.size() + ".");
		}
		return function.call(this, arguments);
	}

	@Override
	public Object visitTernaryExpr(Expr.Ternary expr) {
		Object condition = evaluate(expr.condition);

		if (isTruthy(condition)) return evaluate(expr.antecedent);
		else return evaluate(expr.precedent);
	}

	private Object evaluate(Expr expr) {
		return expr.accept(this);
	}

	private Object execute(Stmt stmt) {
		return stmt.accept(this);
	}

	void executeBlock(List<Stmt> statements, Environment environment) {
		Environment previous = this.environment;
		try {
			this.environment = environment;

			for (Stmt statement : statements) {
				execute(statement);
			}
		}
		finally {
			this.environment = previous;
		}
	}

	@Override
	public Void visitBlockStmt(Stmt.Block stmt) {
		executeBlock(stmt.statements, new Environment(environment));
		return null;
	}

	@Override
	public Void visitExpressionStmt(Stmt.Expression stmt) {
		evaluate(stmt.expression);
		return null;
	}

	@Override
	public Void visitFunctionStmt(Stmt.Function stmt) {
		MidasFunction function = new MidasFunction(stmt, environment);
		environment.define(stmt.name.lexeme, function);
		return null;
	}

	@Override
	public Void visitIfStmt(Stmt.If stmt) {
		if (isTruthy(evaluate(stmt.condition))) {
			execute(stmt.thenBranch);
		}
		else if (stmt.elseBranch != null) {
			execute(stmt.elseBranch);
		}
		return null;
	}

	@Override
	public Void visitWhileStmt(Stmt.While stmt) {
		try {
			while (isTruthy(evaluate(stmt.condition))) {
				execute(stmt.body);
			}
		}
		catch (BreakException ex) {
			// Do nothing.
		}
		return null;
	}

	@Override
	public Void visitBreakStmt(Stmt.Break stmt) {
		throw new BreakException();
	}

	@Override
	public Void visitPrintStmt(Stmt.Print stmt) {
		Object value = evaluate(stmt.expression);
		System.out.println(stringify(value));
		return null;
	}

	@Override
	public Void visitReturnStmt(Stmt.Return stmt) {
		Object value = null;
		if (stmt.value != null) value = evaluate(stmt.value);

		throw new Return(value);
	}

	@Override
	public Void visitVarStmt(Stmt.Var stmt) {
		Object value = null;
		if (stmt.initializer != null) {
			value = evaluate(stmt.initializer);
		}

		environment.define(stmt.name.lexeme, value);
		return null;
	}

	@Override
	public Object visitAssignExpr(Expr.Assign expr) {
		Object value = evaluate(expr.value);

		environment.assign(expr.name, value);
		return value;
	}
}