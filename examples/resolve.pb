var a = "global";

do
    fun printA() do
        print a;
    end

    printA();
    var a = "block";
    printA();
end
