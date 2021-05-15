

/* Your test program. */

// This program tests struct and array in global or local scope

#define long long long

long a[25];

struct Point {
    long a;
    long b;
} b;

void main()
{
    long c;
    long local_array[25];

    c = 4;
    a[c] = c;
    local_array[c] = 24523;

    b.a = c;
    b.b = 2424;
}