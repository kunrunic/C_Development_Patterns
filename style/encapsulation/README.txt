ADD TEST FILE


>>> Case. notEncapsulation
# 6 "other_logic.c" 2
# 1 "public.h" 1

typedef void* tEncapsulation;

tEncapsulation* new();
void destory(tEncapsulation *st) ;
# 7 "other_logic.c" 2

int main(int argc, char **argv) {

    tEncapsulation* cap = new();
    destory(cap);
    return 0;
}

>>> Case. encapsulation 
# 6 "other_logic.c" 2
# 1 "public.h" 1



struct sEncapsulation {
    int idx;
    char name[10];
};
typedef struct sEncapsulation tEncapsulation;

tEncapsulation* new();
void destory(tEncapsulation *st) ;
# 7 "other_logic.c" 2

int main(int argc, char **argv) {

    tEncapsulation* cap = new();
    destory(cap);
    return 0;
}
