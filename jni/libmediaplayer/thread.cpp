#include <android/log.h>
#include "thread.h"

#define TAG "FFMpegThread"

Thread::Thread()
{
	pthread_mutex_init(&mLock, NULL);//初始化 互斥锁
	pthread_cond_init(&mCondition, NULL); // 初始化 条件
}

Thread::~Thread()
{
}

void Thread::start()
{
    handleRun(NULL);
}

void Thread::startAsync()
{
	/**
	 * int pthread_create(pthread_t*restrict tidp,const pthread_attr_t *restrict_attr,void*（*start_rtn)(void*),void *restrict arg);
	 * 若成功则返回0，否则返回出错编号
　　	   返回成功时，由tidp指向的内存单元被设置为新创建线程的线程ID。
       attr参数用于制定各种不同的线程属性。新创建的线程从start_rtn函数的地址开始运行，
       该函数只有一个万能指针参数arg，如果需要向start_rtn函数传递的参数不止一个，
       那么需要把这些参数放到一个结构中，然后把这个结构的地址作为arg的参数传入。
	 */
    pthread_create(&mThread, NULL, startThread, this);

    /**
     * 由 restrict 修饰的指针是最初唯一对指针所指向的对象进行存取的方法，
     * 仅当第二个指针基于第一个时，才能对对象进行存取。
     * 对对象的存取都限定于基于由 restrict 修饰的指针表达式中。
     * 由 restrict 修饰的指针主要用于函数形参，或指向由 malloc() 分配的内存空间。
     * restrict 数据类型不改变程序的语义。
     * 编译器能通过作出 restrict 修饰的指针是存取对象的唯一方法的假设，
     * 更好地优化某些类型的例程。
     */
}

int Thread::wait()
{
	if(!mRunning)
	{
		return 0; //如果线程停止返回0，表示等待成功。
	}
	/**
	 * pthread_join()函数，以阻塞的方式等待thread指定的线程结束。当函数返回时，被等待线程的资源被收回。如果进程已经结束，那么该函数会立即返回。并且thread指定的线程必须是joinable的。
　　	   参数 ：
　　    thread: 线程标识符，即线程ID，标识唯一线程。
　　    retval: 用户定义的指针，用来存储被等待线程的返回值。
　　    返回值 ： 0代表成功。 失败，返回的则是错误号
	 */
    return pthread_join(mThread, NULL);
}

void Thread::stop()
{
}

void* Thread::startThread(void* ptr)
{
    __android_log_print(ANDROID_LOG_INFO, TAG, "starting thread");
	Thread* thread = (Thread *) ptr;
	thread->mRunning = true;
    thread->handleRun(ptr);
	thread->mRunning = false;
	__android_log_print(ANDROID_LOG_INFO, TAG, "thread ended");
}

void Thread::waitOnNotify()
{
	pthread_mutex_lock(&mLock);
	pthread_cond_wait(&mCondition, &mLock);
	pthread_mutex_unlock(&mLock);
}

void Thread::notify()
{
	pthread_mutex_lock(&mLock);
	pthread_cond_signal(&mCondition);
	pthread_mutex_unlock(&mLock);
}

void Thread::handleRun(void* ptr)
{
}
