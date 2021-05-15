#include <stdio.h>
#define LL long long
#define WriteLong(x) printf(" %lld", (long)x)
#define WriteLine() printf("\n")


void main() {
    LL a;
    LL b;
    LL c;

    a = 1;
    LL r5 = 1 == 1;
    if (!(r5)) goto l10;
    b = 2;
    c = 4;
    goto l25;
    a = 1;
    b = 2;
l10:;
    a = 4;
    c = 4;
    a = 4;
l25:;
    LL r12 = a == 3;
    if (!(r12)) goto l18;
    a = 3;
    b = 3;
    c = 4;
    goto l25;
    a = 3;
    b = 3;
l18:;
    b = 5;
    return;
}

