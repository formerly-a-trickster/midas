package jmidas;

import java.util.List;

interface MidasCallable {
	int arity();
	Object call(Interpreter interpreter, List<Object> arguments);
}
