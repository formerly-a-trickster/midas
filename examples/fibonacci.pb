fun fibonacci(n) do
    if (n <= 1) return n;
    return fibonacci(n-2) + fibonacci(n-1);
end

fun computation() do
    var start = clock();

    for (var i = 0; i <= 20; i = i + 1) do
        print i ++ ", " ++ fibonacci(i);
    end

    var finish = clock();
    return finish - start;
end

print "It took " ++ computation() ++ " seconds to compute that.";
