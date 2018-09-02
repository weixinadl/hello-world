#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include "bn.h"

int main(){

    bn_t a = bn_alloc();
    bn_t b = bn_alloc();
    bn_t res = bn_alloc();
    char buf[1000];

    int l=0;
    bn_fromString(a, "0");
    bn_fromString(b, "1");

    char aint[1000];
    char bint[1000];

    strcat(aint, "0");
    strcat(bint, "1");
    /*
    for(int i=0; i<500; i++){
        char* temp1 = int_add(aint, bint);
        strcpy(aint, temp1);
        if(i%100 == 98){
            bn_fromString(res, aint);
            bn_toString(res, buf, 1000);
            printf("%d: %s\n", 2*i+2+1, buf);
        }
        char* temp2 = int_add(aint, bint);
        strcpy(bint, temp2);
        if(i%100 == 98){
            bn_fromString(res, bint);
            bn_toString(res, buf, 1000);
            printf("%d: %s\n", 2*i+2+2, buf);
        }
    }*/


    for(int i=0; i<500; i++){
        bn_add(a, a, b);
        l = bn_toString(a, buf, 1);

        bn_toString(a, buf, l);
        if(i%50 == 48)
        printf("%d: %s\n", 2*i+1+2, buf);
        bn_add(b, a, b);
        l = bn_toString(b, buf, 1);
        bn_toString(b, buf, l);
        if(i%50 == 48)
        printf("%d: %s\n", 2*i+2+2, buf);
    }

    return 0;
}
