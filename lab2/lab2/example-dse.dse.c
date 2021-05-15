#include <stdio.h>
#define LL long long
#define WriteLong(x) printf(" %lld", (long)x)
#define WriteLine() printf("\n")

LL a;

void function_2() {

    return;
}

void function_4() {
    LL b;
    LL c;
    LL d;

    a = 3;
    c = 4;
    d = 1;
    LL r11 = d == 1;
    if (!(r11)) goto l17;
    LL r13 = c + 1;
    c = r13;
    function_2(c);
l17:;
    return;
}

