fun imp_fib(num) do
    var a = 0;
    var b = 1;
    var c = 1;

    for (var i = 0; i < num; i = i + 1) do
        a = b;
        b = c;
        c = a + b;
    end

    return b;
end

fun rec_fib(num) do
    if (num <= 1)
        return 1;
    else
        return rec_fib(num - 2) + rec_fib(num - 1);
end

for (var i = 0; i < 20; i = i + 1) do
    print i ++ ": " ++ rec_fib(i);
end

