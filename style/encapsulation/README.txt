ADD TEST FILE

아래와 같이 캡슐화 케이스 비 캡슐화 케이스의 확연한 차이를 알 수 있다.
캡슐화의 시작이 C를 OOP 스럽게 개발하는 시작이다. (^^ 직접 접근하려고하면 컴파일이 안되요!!)

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
