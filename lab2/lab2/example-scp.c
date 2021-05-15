long a;

void function_1()
{
}

void example(long k, long n)
{
    long s, i, b, c;

    s = 0;
    a = 4;
    c = 5;
    i = 0;

    if (c == 0)
    {
        c = a;
    }

    function_1();

    if (k == 0)
    {
        b = 1;
    }
    else
    {
        b = 2;
    }

    while (i < n)
    {
        s = s + a * b + c;
        i = i + 1;
    }
}