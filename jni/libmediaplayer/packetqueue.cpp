/* mediaplayer.cpp
**
*/

#define TAG "FFMpegPacketQueue"

#include <android/log.h>
#include "packetqueue.h"

PacketQueue::PacketQueue()
{
	pthread_mutex_init(&mLock, NULL);
	pthread_cond_init(&mCondition, NULL);
	mFirst = NULL;
	mLast = NULL;
	mNbPackets = 0;;
    mSize = 0;
    mAbortRequest = false;
}

PacketQueue::~PacketQueue()
{
	flush();
	pthread_mutex_destroy(&mLock);
	pthread_cond_destroy(&mCondition);
}

int PacketQueue::size()
{
	pthread_mutex_lock(&mLock);
    int size = mNbPackets;
    pthread_mutex_unlock(&mLock);
	return size;
}

void PacketQueue::flush()
{
	AVPacketList *pkt, *pkt1;
	
    pthread_mutex_lock(&mLock);
	
    for(pkt = mFirst; pkt != NULL; pkt = pkt1) {
        av_free_packet(&pkt->pkt);
        av_freep(&pkt);
		pkt1 = pkt->next;
    }
    mLast = NULL;
    mFirst = NULL;
    mNbPackets = 0;
    mSize = 0;
	
    pthread_mutex_unlock(&mLock);
}

int PacketQueue::put(AVPacket* pkt)
{
	AVPacketList *pkt1;
	
    /* duplicate the packet */
    if (av_dup_packet(pkt) < 0)
        return -1;
	
    pkt1 = (AVPacketList *) av_malloc(sizeof(AVPacketList));
    if (!pkt1)
        return -1;
    pkt1->pkt = *pkt;
    pkt1->next = NULL;
	
    pthread_mutex_lock(&mLock);
	
    if (!mLast) {//第一个放入空队列中的包
        mFirst = pkt1;
	}
    else {
        mLast->next = pkt1; //
	}
	
    mLast = pkt1;
    mNbPackets++;
    mSize += pkt1->pkt.size + sizeof(*pkt1);
	/**
	 * pthread_cond_signal函数的作用是发送一个信号给另外一个正在处于阻塞等待状态的线程,
	 * 使其脱离阻塞状态,继续执行.如果没有线程处在阻塞等待状态,
	 * pthread_cond_signal也会成功返回。
       使用pthread_cond_signal一般不会有“惊群现象”产生，
       他最多只给一个线程发信号。假如有多个线程正在阻塞等待着这个条件变量的话，
       那么是根据各等待线程优先级的高低确定哪个线程接收到信号开始继续执行。
       如果各线程优先级相同，则根据等待时间的长短来确定哪个线程获得信号。
       但无论如何一个pthread_cond_signal调用最多发信一次。
       但是pthread_cond_signal在多处理器上可能同时唤醒多个线程，
       当你只能让一个线程处理某个任务时，其它被唤醒的线程就需要继续 wait，
       而且规范要求pthread_cond_signal至少唤醒一个pthread_cond_wait上的线程，
       其实有些实现为了简单在单处理器上也会唤醒多个线程.
       另外，某些应用，如线程池，pthread_cond_broadcast唤醒全部线程，
       但我们通常只需要一部分线程去做执行任务，所以其它的线程需要继续wait.
       所以强烈推荐对pthread_cond_wait() 使用while循环来做条件判断.
	 */
	pthread_cond_signal(&mCondition);
    pthread_mutex_unlock(&mLock);
	
    return 0;
	
}

/* return < 0 if aborted, 0 if no packet and > 0 if packet.  */
int PacketQueue::get(AVPacket *pkt, bool block)
{
	AVPacketList *pkt1;
    int ret;
	/**
	 * pthread_mutex_lock()函数锁住由mutex指定的mutex 对象。
	 * 如果mutex已经被锁住，调用这个函数的线程阻塞直到mutex可用为止。
	 * 这跟函数返回的时候参数mutex指定的mutex对象变成锁住状态，
	 * 同时该函数的调用线程成为该mutex对象的拥有者。
	 */
    pthread_mutex_lock(&mLock);
	//这儿是一个无限循环，只有通过break才能退出
    for(;;) {
        if (mAbortRequest) {
            ret = -1;
            break;
        }
		
        pkt1 = mFirst;
        if (pkt1) {
            mFirst = pkt1->next;
            if (!mFirst) //如果下一指向为NULL则队列为空了。
                mLast = NULL; //把队列最后的指针指向空
            mNbPackets--;
            mSize -= pkt1->pkt.size + sizeof(*pkt1);
            *pkt = pkt1->pkt;
            av_free(pkt1);
            ret = 1;
            break;
        } else if (!block) {
            ret = 0;
            break;
        } else {
        	/**
        	 * 条件变量是利用线程间共享的全局变量进行同步的一种机制，
        	 * 主要包括两个动作：一个线程等待;
        	 * 条件变量的条件成立;而挂起；
        	 * 另一个线程使;条件成立;（给出条件成立信号）。
        	 * 为了防止竞争，条件变量的使用总是和一个互斥锁结合在一起。
        	 * 激发条件有两种形式，pthread_cond_signal()激活一个等待该条件的线程，
        	 * 存在多个等待线程时按入队顺序激活其中一个；
        	 * 而pthread_cond_broadcast()则激活所有等待线程。
        	 */
			pthread_cond_wait(&mCondition, &mLock);//这儿判断如果队列中没有包可以解码就等待挂起，通过put方法放入包可激活
		}

    }
    pthread_mutex_unlock(&mLock);
    return ret;
	
}
/**
 * 退出，通过 不能在 队列里取出 包 实现
 */
void PacketQueue::abort()
{
    pthread_mutex_lock(&mLock);
    mAbortRequest = true;
    pthread_cond_signal(&mCondition);
    pthread_mutex_unlock(&mLock);
}
