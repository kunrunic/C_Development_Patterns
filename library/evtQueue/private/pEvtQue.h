
#include <pthread.h>

typedef int (*fEvtNoti)(void *data);
struct sEvtNoti {
    void *data;
    fEvtNoti func;
};
typedef struct sEvtNoti tEvtNoti;

struct sEvtQue {
    int maxCnt;
    int cnt;
    int front;
    int end;
    void **nodes;
};
typedef struct sEvtQue tEvtQue;

struct sEvtThread {
    pthread_mutex_t mutex[1];
    pthread_t thread[1];
    tEvtQue que[1];
    int maxQcnt;
    int isAlive;
    int isCreate;
    int searchCnt;
};
typedef struct sEvtThread tEvtThread;