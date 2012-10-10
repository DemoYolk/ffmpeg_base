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
	
    if (!mLast) {
        mFirst = pkt1;
	}
    else {
        mLast->next = pkt1;
	}
	
    mLast = pkt1;
    mNbPackets++;
    mSize += pkt1->pkt.size + sizeof(*pkt1);
	
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
	
    for(;;) {
        if (mAbortRequest) {
            ret = -1;
            break;
        }
		
        pkt1 = mFirst;
        if (pkt1) {
            mFirst = pkt1->next;
            if (!mFirst)
                mLast = NULL;
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
