fun fib(num) do
    var a = 0;
    var b = 0;
    var c = 1;

    for (var i = 0; i < num; i = i + 1) do
        a = b;
        b = c;
        c = a + b;
    end

    print b;
end

fib(0);
fib(1);
fib(2);
fib(3);
fib(4);
fib(5);
fib(6);
fib(7);
fib(8);
fib(9);
fib(10);

