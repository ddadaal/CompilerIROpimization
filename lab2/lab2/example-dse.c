long a;

void test(long a)
{
}

void func()
{
    long b, c, d;

    a = 3;
    b = 2;
    c = 3;
    c = 4;
    d = 1;

    if (d == 1)
    {
        c = c + 1;
        test(c);
    }
}