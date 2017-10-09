# Midas

Midas is a small programming language, currently in development, with no formal
specification and only experimental implementations.

Midas code can be run by a prototype tree walk interpreter written in Python,
while the proper interpreter will be implemented in C.

## Features

Most notably, Midas has first-class function that close over their declaring
scope, allowing them to express constructs that are usually built into the
language, like objects and generators.

```
# Using a closure like an object
fun makePoint(x, y) do
    fun closure(method) do
        if (method == "x") do
          return x;
        end
        if (method == "y") do
          return y;
        end
        print "Unknown method" ++ method;
    end

    return closure;
end
```

```
var point_a = makePoint(2, 3);
print point_a("x");     # Prints 2.
print point_a("y");     # Prints 3.
# Using closures like a generators
fun makeCounter() do
    var i = 0;
    fun counter() do
      i = i + 1;
      return i;
    end

    return counter;
end

var count = makeCounter();
print count();     # Prints 1.
print count();     # Prints 2.
```

## Try it out

The interpreter requires at least Python 3 to run. It can be run in REPL mode, or
run one of the source files in the "examples" folder.

If running from Windows, open a console window in the root directory and type
the following in order to run the point_distance.pb file:

```
> midas.bat examples\point_distance.pb
```

If running from a \*nix environment, you can use the "midas.sh" script to the
same effect:

```
$ ./midas.sh examples/point_distance.pb
```
