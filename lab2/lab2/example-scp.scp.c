#include <stdio.h>
#define LL long long
#define WriteLong(x) printf(" %lld", (long)x)
#define WriteLine() printf("\n")

LL a;

void function_2(LL k, LL n) {
    LL s;
    LL i;
    LL b;
    LL c;

    s = 0;
    a = 4;
    c = 5;
    i = 0;
    LL r8 = k == 0;
    if (!(r8)) goto l12;
    b = 1;
    goto l13;
l12:;
    b = 2;
l13:;
    LL r13 = i < n;
    if (!(r13)) goto l24;
    LL r16 = 4;
    LL r17 = 4 * b;
    LL r18 = s + r17;
    LL r19 = r18 + 5;
    s = r19;
    LL r21 = i + 1;
    i = r21;
    goto l13;
l24:;
    return;
}

