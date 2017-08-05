package com.jmidas.midas;

class Return extends RuntimeException {
	final Object value;

	Return(Object value) {
		super(null, null, false, false);
		this.value = value;
	}
}
