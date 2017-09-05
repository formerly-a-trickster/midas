fun fibonacci(n) do
    if (n <= 1) return n;
    return fibonacci(n-2) + fibonacci(n-1);
end

for (var i = 0; i <= 20; i = i + 1) do
    print i ++ ", " ++ fibonacci(i);
end