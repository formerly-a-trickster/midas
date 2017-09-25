fun heronSqrt(n) do
    var guess = 1;

    for var i = 0; i < 10; i = i + 1 do
        guess = (guess +  n / guess ) / 2;
    end

    return guess;
end

fun printSqrt(n) do
    print "The square root of " ++ n
       ++ " is " ++ heronSqrt(n);
end

printSqrt(256);
printSqrt(16);
printSqrt(100);
printSqrt(2);
