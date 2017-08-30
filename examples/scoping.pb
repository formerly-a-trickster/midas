var a = "global a";
var b = "global b";
var c = "global c";

do
    var a = "outer a";
    var b = "outer b";

    do
        var a = "inner a";

        print a;
        print b;
        print c;
        print "========";
    end

    print a;
    print b;
    print c;
    print "========";
end

print a;
print b;
print c;
