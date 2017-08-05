fun makeCounter() do
    var i = 0;

    fun count() do
        i = i + 1;
        return i;
    end

    return count;
end

var count = makeCounter();

for (var i = 0; i < 10; i = i + 1) do
    print count();
end
