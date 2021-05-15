

/* Your test program. */

// This program tests the call and return machenism as well as operators

#define long long long

long results[9];

void called_function(long a, long b)
{
    results[0] = a + b;
    results[1] = a - b;
    results[2] = a * b;
    results[3] = a / b;
    results[4] = a % b;
    results[5] = -a;
}

void main()
{
    long c;

    called_function(2, 4);

    c = 4;
    called_function(c, 6);

    called_function(c, c);
}