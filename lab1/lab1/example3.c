

/* Your test program. */

// This tests store, load, WriteLine and WriteLong

#define WriteLine() printf("\n");
#define WriteLong(x) printf(" %lld", (long)x);
#define long long long

void main()
{
    long array[4];
    array[0] = 0;
    array[1] = 1;
    array[2] = 2;
    array[3] = 3;

    array[2] = array[3];
    WriteLine();
    WriteLong(array[1]);
}