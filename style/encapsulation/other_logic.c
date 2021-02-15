

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "public.h"

int main(int argc, char **argv) {

    tEncapsulation* cap = new();
    init(cap, 100, "encapsulation");
    // can't call __display() function because it's hidden.
    __display(cap); // << compile fail. (warning and error)
    // not exist public.h but compile successed.
    display(cap);  // << compile success. (warning)
    destory(cap); 
    return 0;
}