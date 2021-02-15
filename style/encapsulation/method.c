
#include <stdio.h>
#include <stdlib.h>
#include "private.h"

// __display is Private Function
static void __display(tEncapsulation* data) {
    printf("idx:%d, name:%s\n", data->idx, data->name);
}
 
 void display(tEncapsulation* data) {
    printf("idx:%d, name:%s\n", data->idx, data->name);
 }

// init, new, destory is Public Functions
void init(tEncapsulation *st, int idx, char *name) {
    st->idx = idx;
    snprintf(st->name, sizeof(st->name), "%s", name);
    __display(st);
}

tEncapsulation* new() {
    return (tEncapsulation*)malloc(sizeof(tEncapsulation)); // allocate memory
}

void destory(tEncapsulation *st) {
    if( st == NULL) { return; }; // check to input args
    free(st); // allocate memory free
}