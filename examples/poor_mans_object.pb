fun makePoint(x, y) do
    fun closure(method) do
        if (method == "x") return x;
        if (method == "y") return y;
        print "Unknown method " + method;
    end

    return closure;
end

var point = makePoint(2, 3);
