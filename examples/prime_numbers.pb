fun isPrime(num) do
    if num <= 2 do
        return true;
    end
    if num % 2 == 0 do
        return false;
    end

    for var i = 3; i < num; i = i + 2 do
        if num % i == 0 do
            return false;
        end
    end

    return true;
end

print "Prime numbers up to 100:";
for var i = 1; i <= 100; i = i + 1 do
    if isPrime(i) do
        print i;
    end
end
