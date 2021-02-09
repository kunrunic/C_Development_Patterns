
#include <stdlib.h>
#include "private.h"

tEncapsulation* new() {
    return (tEncapsulation*)malloc(sizeof(tEncapsulation)); // allocate memory
}

void destory(tEncapsulation *st) {
    if( st == NULL) { return; }; // check to input args
    free(st); // allocate memory free
}