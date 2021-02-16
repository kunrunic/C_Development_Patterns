


typedef int (*fEvtNoti)(void *data);

int evtQue_create( int maxQcnt );
int evtQue_destory( );
int evtQue_notify( int idx, void *data, fEvtNoti func );
int evtQue_getQcnt( int idx );
int evtQue_getMaxQcnt( int idx );
int evtQue_isCreate( int idx );