# Generator-like expression using a clojure
fun fibGenerator() do
    var a = 1;
    var b = 1;
    var c = 0;

    fun next() do
        c = a + b;
        a = b;
        b = c;

        return c;
    end

    return next;
end

var fibGen = fibGenerator();

for (var i = 2 ; i <= 100; i = i + 1) do
    print i ++ ", " ++ fibGen();
end
