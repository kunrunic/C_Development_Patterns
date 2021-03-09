
#include "private/pEvtQue_def.h"
#include "private/pEvtQue.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
 


/********************
  Private Functions
********************/
#define EVT_MAX_THREAD_SIZE 10
static tEvtThread* _evtQue_instances( int idx ) {
	static tEvtThread _singletone[EVT_MAX_THREAD_SIZE];
	if( idx >= EVT_MAX_THREAD_SIZE ) return NULL;
	return &_singletone[idx];
}

/**
 * Queue의 맨 앞에서 데이터를 가져오고 해당 데이터를 queue로 부터 삭제하는 함수
 * @param queue의 인스턴스
 * @return queue의 맨 앞 데이터
 */
static void* _evtQue_popFront( tEvtQue* queue ) {
	if( queue->cnt <= 0 ) { return NULL; }
	if( queue->nodes == NULL ) { return NULL; }
	void* data = queue->nodes[queue->front++];
	if( queue->front >= queue->maxCnt ) queue->front = 0;
	--queue->cnt;
	if( queue->cnt <= 0 ) queue->front = queue->end = 0;
	return data;
}

/**
 * Event 가 있는 경우 event를 꺼내고 event의 callback을 실행시키는 함수
 * @param ethr event thread 인스턴스
 * @return event 감지 성공 실패 여부
 */
static int _evtQue_epoll( tEvtThread* ethr )
{
	pthread_mutex_lock(ethr->mutex);
	tEvtNoti* noti= _evtQue_popFront( ethr->que );
	pthread_mutex_unlock(ethr->mutex);

	if( noti == NULL ) return eEvtQue_NO_DATA;
	if( noti->func ) noti->func( noti->data );
	if( noti ) free( noti );

	return eEvtQue_SUCCESS;
}

/**
 * Event를 기다리는 함수
 * @param arg event thread 인스턴스
 * @return event thread 인스턴스
 */
static void* _evtQue_wait( void *arg )
{
	int rv=eEvtQue_SUCCESS;
	struct timeval tv[1];
	tEvtThread *ethr = (tEvtThread*)arg;
	ethr->searchCnt = 0;
	while( ethr->isAlive ) {
		rv = _evtQue_epoll( ethr );
		if( rv == eEvtQue_NO_DATA ) {
			if( ethr->searchCnt>= EVT_DEFAULT_QCNT ) {
				tv->tv_sec = 0;
				tv->tv_usec = 500;
				select(0, NULL, NULL, NULL, tv);
			} else {
				ethr->searchCnt++;
				if( ethr->searchCnt >= 100 ) {
					tv->tv_sec = 0;
					tv->tv_usec = ethr->searchCnt / 100;
					select(0, NULL, NULL, NULL, tv);
				}
			}
			continue;
		}
		ethr->searchCnt = 0;
	}

	// tEvtThread의 현재 사용중인 Que 개수가
	// 논리적인 max que 개수를 넘어간 상태이면 그냥 나간다.
	if( ethr->maxQcnt <= ethr->que->cnt ) return arg;

	// 남은 로그를 처리한다.
	int i=0;
	int reserved = ethr->que->cnt;
	for( i=0; i < reserved; i++ ) {
		_evtQue_epoll( ethr );
	}
	return arg;
}

/*
 * @internal event polling 할 thread를 생성한다.
 * @param ethr event thread 인스턴스
 * @return 성공/실패(thread 생성 실패)
 */
static int _evtQue_start( tEvtThread* ethr )
{
	ethr->isAlive = 1;
	pthread_create( ethr->thread, NULL, _evtQue_wait, (void*)ethr );
	if( ethr->thread == NULL ) return eEvtQue_THREAD_CREATE_FAIL;
	return eEvtQue_SUCCESS;
}

/**
 * Event를 저장할 queue를 생성하는 함수
 * @param queue queue 인스턴스
 * @param size 생성할 size
 * @return 성공/실패(파라메터 오류, 메모리 할당 실패)
 */
static int _evtQue_queInit( tEvtQue* queue, int size )
{
	if( queue == NULL ) return eEvtQue_INVALID_PARAM;
	if( size <= 0 ) size = EVT_DEFAULT_QCNT;

	queue->nodes = (void**)malloc( sizeof(void*) * size );
	if( queue->nodes == NULL ) return eEvtQue_MEM_ALLOC_FAIL;
	queue->maxCnt = size;
	queue->cnt = 0;
	queue->front = 0;
	queue->end = 0;

	return eEvtQue_SUCCESS;
}

/**
 * Queue를 소멸하는 함수
 * @param queue queue 인스턴스
 * @return 성공
 */
static int _evtQue_queDestroy( tEvtQue* queue )
{
	if( queue == NULL ) return eEvtQue_SUCCESS;
	if( queue->nodes == NULL ) return eEvtQue_SUCCESS;

	queue->maxCnt = 0;
	if( queue->nodes ) free( queue->nodes );
	queue->nodes = NULL;
	queue->cnt = 0;
	queue->front = 0;
	queue->end = 0;
	return eEvtQue_SUCCESS;
}

/**
 * Event thread를 초기화 함수
 * @param ethr event thread 인스턴스
 * @return 성공/실패(파라메터 오류, 기타 호출함수의 실패 사유)
 */
static int _evtQue_init( tEvtThread* ethr, int mqcnt )
{
	int rv=eEvtQue_SUCCESS;
	if( ethr == NULL ) return eEvtQue_INVALID_PARAM;
	if( ethr->isCreate == 1 ) return eEvtQue_ALREADY_CREATE;

	//struct init
	memset( ethr, 0x0, sizeof(tEvtThread) );

	// mutex init
	pthread_mutex_init( ethr->mutex, NULL );

	// event queue init
	rv = _evtQue_queInit( ethr->que, EVT_DEFAULT_QCNT );
	if( rv < eEvtQue_SUCCESS ) return rv;

	ethr->maxQcnt = mqcnt <= 0 ? EVT_DEFAULT_QCNT : mqcnt;

	// event thread start
	rv = _evtQue_start( ethr );
	if( rv < eEvtQue_SUCCESS ) {
		_evtQue_queDestroy( ethr->que );
		return rv;
	}
	ethr->isCreate= 1;

	return eEvtQue_SUCCESS;
}

/* 전달된 size를 전달된 boundary로 정렬할 경우의 size를 반환한다. */
#define EVT_ALIGN(size, boundary)  (((size)+((boundary)-1)) & ~((boundary)-1))

/**
 * Queue를 전달되 크기로 증가하는 함수
 * 만약 전달된 값이 현재 크기보다 작으면 아무 동작도 하지 않는다.
 * @param queue queue의 인스턴스
 * @param rsize resize 하고자하는 크기
 * @return 성공/실패(메모리 할당 실패)
 */
static int _evtQue_queResize( tEvtQue* queue, int rsize )
{
	int i, exsize;
	if( queue->maxCnt < rsize ) {
		rsize = EVT_ALIGN(rsize, 256);
		void **nodes = (void**)realloc( queue->nodes, rsize * sizeof(void*) );
		if( nodes == NULL ) return eEvtQue_MEM_ALLOC_FAIL;
		queue->nodes = nodes;
		if( queue->front > queue->end ) {
			exsize = rsize - queue->maxCnt;
			for( i=queue->cnt-1; i >= queue->front; --i) queue->nodes[i+exsize] = queue->nodes[i];
			queue->front += exsize;
		}
		queue->maxCnt = rsize;
	}
	return eEvtQue_SUCCESS;
}

/**
 * Queue의 맨 뒤에 데이터를 추가하는 함수
 * @param queue queue의 인스턴스
 * @param data 추가할 데이터
 * @return 성공/실패(메모리 이상, 메모리 부족)
 */
static int _evtQue_pushBack( tEvtQue* queue, void *data )
{
	int rv=eEvtQue_SUCCESS;
	if( queue->nodes == NULL ) return eEvtQue_MEM_INVALID;
	if( queue->maxCnt <= queue->cnt ) {
		rv = _evtQue_queResize( queue, queue->cnt *2 );
		fprintf( stderr, "---- event thread queue resize = %d, mqcnt = %d\n", rv, queue->maxCnt );
		if( rv < eEvtQue_SUCCESS ) return rv;
	}
	if( queue->cnt > 0 ) queue->end++;
	if( queue->end >= queue->maxCnt ) queue->end = 0;
	queue->nodes[queue->end] = data;
	++queue->cnt;

	return eEvtQue_SUCCESS;
}

/**
 * Queue의 데이터를 추가
 * @param ethr event thread 인스턴스
 * @param noti noti 될 메시지
 * @return 성공/실패(메모리 이상, 메모리 부족)
 */
static int _evtQue_push( tEvtThread* ethr, tEvtNoti* noti )
{
	int rv=eEvtQue_SUCCESS;
	pthread_mutex_lock( ethr->mutex );
	rv = _evtQue_pushBack( ethr->que, (void*)noti );
	pthread_mutex_unlock( ethr->mutex );
	return rv;
}

/**
 * Event thread의 종료 요청
 * @param ethr event thread 인스턴스
 * @return 성공
 */
static int _evtQue_stop( tEvtThread* ethr )
{
	int rv=eEvtQue_SUCCESS;
	if( ethr == NULL ) return eEvtQue_SUCCESS;
	if( ethr->isAlive == 0 ) return eEvtQue_SUCCESS;

	ethr->isAlive = 0;
	_evtQue_push( ethr, NULL );
	return eEvtQue_SUCCESS;
}

/**
 * Event thread의 종료를 대기
 * @param ethr event thread 인스턴스
 * @return 성공/실패(pthread_join 실패)
 */
static int _evtQue_join( tEvtThread* ethr )
{
	int rv=eEvtQue_SUCCESS;
	if( ethr == NULL ) return eEvtQue_SUCCESS;
	if( *ethr->thread == 0) return eEvtQue_SUCCESS;

	rv = pthread_join( *ethr->thread, NULL );
	*ethr->thread = 0;
	if( rv != 0 ) return eEvtQue_ERROR;
	return eEvtQue_SUCCESS;
}

/**
 * Event thread의 사용을 종료
 * @param ethr event thread 인스턴스
 * @return 성공/실패(queue 소멸 실패)
 */
static int _evtQue_final( tEvtThread* ethr )
{
	int rv=eEvtQue_SUCCESS;
	if( ethr == NULL ) return eEvtQue_SUCCESS;

	rv = _evtQue_queDestroy( ethr->que );
	if( rv < eEvtQue_SUCCESS ) return rv;
	return eEvtQue_SUCCESS;
}

/*******************
  Public Functions
*******************/
/**
 * Event thread pool에서 사용할 idx의 인스턴스를 생성한다.
 * @param idx 생성할 event thread 인스턴스 idx
 * @return 성공/실패(인스턴스 range over, event thread 초기화 실패)
 */
int evtQue_create( int idx, int mqcnt )
{
	int rv=eEvtQue_SUCCESS;

	tEvtThread* ethr = _evtQue_instances( idx );
	if( ethr == NULL ) return eEvtQue_ERROR;

	rv = _evt_thread_init( ethr, mqcnt );
	if( rv < eEvtQue_SUCCESS && rv != eEvtQue_ALREADY_CREATE ) return rv;

	return eEvtQue_SUCCESS;
}

/**
 * 요청한 idx의 인스턴스를 event thread pool에서 소멸
 * @param idx 소멸할 event thread 인스턴스 idx
 * @return 성공/실패(event thread 종료 실패)
 */
int evtQue_destroy( int idx ) {
	tEvtThread* ethr = _evtQue_instances( idx );
	if( ethr == NULL ) return eEvtQue_SUCCESS;
	int rv=eEvtQue_SUCCESS;
	if( (rv=_evtQue_stop(ethr)) < eEvtQue_SUCCESS ) { return rv; }
	if( (rv=_evtQue_join(ethr)) < eEvtQue_SUCCESS ) { return rv; }
	if( (rv=_evtQue_final(ethr)) < eEvtQue_SUCCESS ) { return rv; }
	pthread_mutex_destroy( ethr->mutex );
	ethr->isCreate= 0;
	return eEvtQue_SUCCESS;
}

/**
 * 요청한 idx의 event thread에게 event를 전달
 * @param idx 전달할 event thread 인스턴스 idx
 * @param data 전달할 data
 * @param func event 감지한 경우 실행할 callback function
 * @return 성공/실패(메모리 할당 실패, push 실패)
 */
int evtQue_notify( int idx, void *data, fEvtNoti func )
{
	tEvtThread* ethr = _evtQue_instances( idx );
	if( ethr == NULL ) return eEvtQue_INVALID_PARAM;
	// evt thread의 현재 사용중인 que 개수가
	// 논리적인 max que 개수를 넘어가면 들어오는 로그는 모두 버린다.
	if( evt_getMaxQcnt(idx) <= evtQue_getQcnt(idx) ) return eEvtQue_QUEUE_FULL;

	tEvtNoti* noti = malloc( sizeof(tEvtNoti) );
	if( noti == NULL ) return eEvtQue_MEM_ALLOC_FAIL;
	noti->data = data;
	noti->func = func;
	int rv = _evtQue_push( ethr, noti );
	if( rv < eEvtQue_SUCCESS ) {
		free(noti);
	}
	return rv;
}

/**
 * 요청한 idx의 인스턴스의 max queue count
 * @param idx event thread 인스턴스 idx
 * @return 성공(max queue count 반환)/실패(0 반환)
 */
int evtQue_getMaxQcnt( int idx )
{
	tEvtThread* ethr = _evtQue_instances( idx );
	if( ethr == NULL ) return eEvtQue_SUCCESS;
	return ethr->maxQcnt;
}

/**
 * 요청한 idx의 인스턴스의 current queue count
 * @param idx event thread 인스턴스 idx
 * @return 성공(current queue count 반환)/실패(0 반환)
 */
int evtQue_getQcnt( int idx )
{
	tEvtThread* ethr = _evtQue_instances( idx );
	if( ethr == NULL ) return eEvtQue_SUCCESS;
	return ethr->que->cnt;
}

/**
 * 요청한 idx의 인스턴스의 thread create 생성 여부
 * @param idx event thread 인스턴스 idx
 * @return 성공(1)/실패(0) 반환
 */
int evtQue_isCreate( int idx )
{
	tEvtThread* ethr = _evtQue_instances( idx );
	if( ethr == NULL ) return 0;
	return ethr->isCreate;
}

/**
 * Section, title을 이용하여 값을 찾음
 * @param cfile configuration file
 * @param section configuration section 이름
 * @param log_name thread on 할 log 이름
 * @return mqcnt
 */
int evtQue_getConf( const char *cfile, const char *section, const char *log_name )
{
    char buf[8192]={0,};
    char *data[3]={0,};
    int rv=0;

	if( log_name == NULL ) return eEvtQue_ERROR;

    FILE *fp = fopen( cfile, "r" );
    if( fp == NULL ) {
        fprintf(stderr, "Can't open \'%s\' file.\n", cfile );
		return eEvtQue_ERROR;
    }

    rv = util_conf_goto_section( fp, section );
    if( rv < eEvtQue_SUCCESS ) goto final;

	while( util_conf_read_next_line( fp, buf, sizeof(buf)) ) {
		rv = util_conf_split_data( buf, ";", data, 2, 2 );
		if( rv != eEvtQue_SUCCESS ) {
			fprintf(stderr, "invalid format data(%s) err=%d\n", buf, rv );
			continue;
		}

		/*
		   data[0] log name
		   data[1] max qcnt
		 */
		if( data[0] == NULL || (data[0] && strcasecmp(data[0], log_name)) ) continue;
		if( data[1] != NULL) rv = strtol(data[1], NULL, 0);
		else rv = -1;
		goto final;
	}

	rv = eEvtQue_ERROR;
final:
    fclose( fp );
    return rv;
}
