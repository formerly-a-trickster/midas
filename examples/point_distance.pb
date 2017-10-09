# We can use closures as a kind of object
fun makePoint(x, y) do
    fun closure(method) do
        if (method == "x") do
            return x;
        end
        if (method == "y") do
            return y;
        end
        if (method == "str") do
            return "(" ++ x ++ ", " ++ y ++ ")";
        end
        print "Unknown method " ++ method;
    end
    return closure;
end

# Heron's method of computing square roots
fun heronSqrt(num) do
    var guess = 1;
    for var i = 0; i < 10; i = i + 1 do
        guess = (guess + num / guess) / 2;
    end
    return guess;
end

fun pointDistance(p1, p2) do
    var dX = p2("x") - p1("x");
    var dY = p2("y") - p1("y");
    return heronSqrt(dX * dX + dY * dY);
end

var point_a = makePoint(2, 3);
var point_b = makePoint(10, 5);
print "Point A is at " ++ point_a("str") ++
    ". Point B is at " ++ point_b("str") ++ ".";

var distance = pointDistance(point_a, point_b);
print "The distance between them is " ++ distance;

