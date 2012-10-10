#ifndef FFMPEG_THREAD_H
#define FFMPEG_THREAD_H

#include <pthread.h>
/**
 * 线程
 */
class Thread
{
public:
	Thread();
	~Thread();

	void						start(); //启动
    void						startAsync(); //异步启动
    int							wait(); //等待

    void 						waitOnNotify(); //等待唤醒
    void						notify(); //唤醒
    virtual void				stop();

protected:
    bool						mRunning; //是否在跑

    virtual void                handleRun(void* ptr); //控制线程
	
private:
    pthread_t                   mThread; //线程id
    pthread_mutex_t     		mLock; //线程锁
    pthread_cond_t				mCondition; //线程条件

	static void*				startThread(void* ptr); //开始线程
};

#endif //FFMPEG_DECODER_H
