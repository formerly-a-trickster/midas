fun isPrime(num) do
    if (num <= 2) return true;
    if (num % 2 == 0) return false;

    for (var i = 3; i < num; i = i + 2) do
        if (num % i == 0) return false;
    end

    return true;
end

print "Prime numbers up to 100:";
for (var i = 1; i <= 100; i = i + 1)
    if (isPrime(i))
        print i;