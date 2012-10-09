#ifndef FFMPEG_MEDIAPLAYER_H
#define FFMPEG_MEDIAPLAYER_H

#include <pthread.h>

#include <jni.h>
#include <android/Errors.h>

#include "decoder_audio.h"
#include "decoder_video.h"

#define FFMPEG_PLAYER_MAX_QUEUE_SIZE 10 //最大解码队列 为 10个

using namespace android;

enum media_event_type {
    MEDIA_NOP               = 0, // interface test message （测试 信息 的 接口）
    MEDIA_PREPARED          = 1, // 媒体准备
    MEDIA_PLAYBACK_COMPLETE = 2, // 媒体播放完毕
    MEDIA_BUFFERING_UPDATE  = 3, // 媒体缓冲更新
    MEDIA_SEEK_COMPLETE     = 4, // 媒体查找完毕
    MEDIA_SET_VIDEO_SIZE    = 5, // 媒体设置视频大小
    MEDIA_ERROR             = 100, // 媒体错误
    MEDIA_INFO              = 200, // 媒体信息
};

// Generic error codes for the media player framework.  Errors are fatal, the
// playback must abort.（媒体播放器框架的通用错误代码，错误是致命的，播放必须被取消）
//
// Errors are communicated back to the client using the
// MediaPlayerListener::notify method defined below.（错误通过 MediaPlayerListener::notify 方法（在下面定义的） 被传送到客户端）
// In this situation, 'notify' is invoked with the following:（在这个情况下，‘notify’被调用时需要做以下处理：）
//   'msg' is set to MEDIA_ERROR.       （‘msg’ 被设置为错误信息）
//   'ext1' should be a value from the enum media_error_type. （‘ext1’ 需要设置为 media_error_type的一个值）
//   'ext2' contains an implementation dependant error code to provide
//          more details. Should default to 0 when not used. （‘ext2’ 包含一个完整扩展的错误代码 提供更多的信息。如果没用需要设置为默认值0）
//


// The codes are distributed as follow: （代码 描述如下：）
//   0xx: Reserved （备用的）
//   1xx: Android Player errors. Something went wrong inside the MediaPlayer. （android播放器错误。MediaPlayer的内部出现了某些问题）
//   2xx: Media errors (e.g Codec not supported). There is a problem with the
//        media itself. （媒体错误（例如：编码不支持）。这是 媒体自己有问题。）
//   3xx: Runtime errors. Some extraordinary condition arose making the playback
//        impossible. （运行错误。某些出乎意料的条件 产生了播放不可用）
//

enum media_error_type {
	// 0xx
    MEDIA_ERROR_UNKNOWN = 1,
    // 1xx
    MEDIA_ERROR_SERVER_DIED = 100,
    // 2xx
    MEDIA_ERROR_NOT_VALID_FOR_PROGRESSIVE_PLAYBACK = 200,
};


// Info and warning codes for the media player framework.  These are non fatal,
// the playback is going on but there might be some user visible issues. （媒体播放器框架 的信息和警告代码 。这些代码都不会致命，播放会继续 但是可能有一些用户体验问题）
//
// Info and warning messages are communicated back to the client using the
// MediaPlayerListener::notify method defined below.  （信息和警告 通过 MediaPlayerListener::notify 方法（在下面定义的） 被传送到客户端）
//In this situation, 'notify' is invoked with the following: （在这个情况下，‘notify’被调用时需要做以下处理：）
//   'msg' is set to MEDIA_INFO.  ('msg' 设置 媒体信息)
//   'ext1' should be a value from the enum media_info_type. （‘ext1’ 设置为 media_info_type的一个值）
//   'ext2' contains an implementation dependant info code to provide
//          more details. Should default to 0 when not used. （‘ext2’ 包含一个完整扩展的错误代码 提供更多的信息。如果没用需要设置为默认值0）
//
// The codes are distributed as follow: （代码规则 描述如下：）
//   0xx: Reserved （备用）
//   7xx: Android Player info/warning (e.g player lagging behind.) （比如 播放器 滞后）
//   8xx: Media info/warning (e.g media badly interleaved.) （比如 媒体 严重交错）
//
enum media_info_type {
    // 0xx
    MEDIA_INFO_UNKNOWN = 1,
    // 7xx
    // The video is too complex for the decoder: it can't decode frames fast
    // enough. Possibly only the audio plays fine at this stage. （视频多余解码器太复杂：导致 不能解码帧的速度足够快 。可能只有音频播放正常 在这个情况）
    MEDIA_INFO_VIDEO_TRACK_LAGGING = 700,
    // 8xx
    // Bad interleaving means that a media has been improperly interleaved or not
    // interleaved at all, e.g has all the video samples first then all the audio
    // ones. Video is playing but a lot of disk seek may be happening. （严重加错 的意思 是 一个媒体被不对的交错 或者 完全没有交错，例如 先找到视频然后再找到音频。视频正在播放但是同时有很多的硬盘寻址操作在发生可能会引起这个问题）
    MEDIA_INFO_BAD_INTERLEAVING = 800,
    // The media is not seekable (e.g live stream). （媒体不可被 指定播放）
    MEDIA_INFO_NOT_SEEKABLE = 801,
    // New media metadata is available.  （新的媒体 元数据 可用）
    MEDIA_INFO_METADATA_UPDATE = 802,

    MEDIA_INFO_FRAMERATE_VIDEO = 900, //（我草，上面完全没有提到9xx的代码表示什么意思，太坑跌了！）
    MEDIA_INFO_FRAMERATE_AUDIO, //（我草，上面完全没有提到9xx的代码表示什么意思，太坑跌了！）
};

// 媒体播放器的状态 代码

enum media_player_states {
    MEDIA_PLAYER_STATE_ERROR        = 0, // 媒体播放器 状态错误
    MEDIA_PLAYER_IDLE               = 1 << 0, // 媒体播放器 状态空闲
    MEDIA_PLAYER_INITIALIZED        = 1 << 1, // 状态 初始化完毕
    MEDIA_PLAYER_PREPARING          = 1 << 2, // 准备中
    MEDIA_PLAYER_PREPARED           = 1 << 3, // 准备完毕
	MEDIA_PLAYER_DECODED            = 1 << 4, // 解码完毕
    MEDIA_PLAYER_STARTED            = 1 << 5, // 开始完毕
    MEDIA_PLAYER_PAUSED             = 1 << 6, // 暂停完毕
    MEDIA_PLAYER_STOPPED            = 1 << 7, // 停止完毕
    MEDIA_PLAYER_PLAYBACK_COMPLETE  = 1 << 8 // 播放完毕
};

// ----------------------------------------------------------------------------
// ref-counted object for callbacks
// 媒体播放器的 状态 监听器类
class MediaPlayerListener
{
public:
	// 抽象方法； 根据上面的提示 应该是 唤醒 客户端对应的监听器并传递 相应的信息
    virtual void notify(int msg, int ext1, int ext2) = 0;
};

//媒体播放器
class MediaPlayer
{
public:
    MediaPlayer();
    ~MediaPlayer();
	status_t        setDataSource(const char *url); //设置 资源数据 ，一个包含 资源地址的字符串
	status_t        setVideoSurface(JNIEnv* env, jobject jsurface); // 设置 视频的展示页面，从理论上推测应该使用了android源码的surface类
	status_t        setListener(MediaPlayerListener *listener); // 设置 播放器的监听器
	status_t        start(); //开始
	status_t        stop();  // 停止
	status_t        pause();
	bool            isPlaying(); // 是否在播放
	status_t        getVideoWidth(int *w); //得到视频的宽度
	status_t        getVideoHeight(int *h); // 得到视频的高度
	status_t        seekTo(int msec); // 指定查找到的 时间
	status_t        getCurrentPosition(int *msec); //得到 当前播放的位置
	status_t        getDuration(int *msec); //得到 媒体的 播放时间
	status_t        reset(); // 重置
	status_t        setAudioStreamType(int type); // 设置音频流的类型
	status_t		prepare(); //准备
	void            notify(int msg, int ext1, int ext2); //唤醒
//    static  sp<IMemory>     decode(const char* url, uint32_t *pSampleRate, int* pNumChannels, int* pFormat);
//    static  sp<IMemory>     decode(int fd, int64_t offset, int64_t length, uint32_t *pSampleRate, int* pNumChannels, int* pFormat);
//    static  int             snoop(short *data, int len, int kind);
//            status_t        invoke(const Parcel& request, Parcel *reply);
//            status_t        setMetadataFilter(const Parcel& filter);
//            status_t        getMetadata(bool update_only, bool apply_filter, Parcel *metadata);
	status_t        suspend(); //中断
	status_t        resume(); // 恢复

private:
	status_t					prepareAudio(); //准备音频
	status_t					prepareVideo(); // 准备视频
	bool						shouldCancel(PacketQueue* queue); //是否取消
	static void					ffmpegNotify(void* ptr, int level, const char* fmt, va_list vl); // ffmpeg唤醒
	static void*				startPlayer(void* ptr); // 开始播放器

	static void 				decode(AVFrame* frame, double pts); // 解码 帧数据
	static void 				decode(int16_t* buffer, int buffer_size); // 解码 缓冲的数据

	void						decodeMovie(void* ptr); // 解码 电影？
	
	double 						mTime; // 时间
	pthread_mutex_t             mLock; // 锁定时间
	pthread_t					mPlayerThread; // 播放器线程 是一个long型的值（不明白？难道是线程名称？）
	PacketQueue*				mVideoQueue; // 视频包队列，是一个类
    //Mutex                       mNotifyLock;
    //Condition                   mSignal;
    MediaPlayerListener*		mListener; // 播放器监听器
    AVFormatContext*			mMovieFile; //流域，用于寻找文件中的流，包含完整流信息
    int 						mAudioStreamIndex; // 音频流索引
    int 						mVideoStreamIndex; // 视频流索引
    DecoderAudio*				mDecoderAudio; // 音频解码器
	DecoderVideo*             	mDecoderVideo; // 视频解码器
	AVFrame*					mFrame; // 幀
	struct SwsContext*			mConvertCtx; // 目测使用来 转换 视频用的，目前应该用不到

    void*                       mCookie; //一个指向任意类型的指针
    media_player_states         mCurrentState; // 播放器当前状态
    int                         mDuration; // 媒体总播放时间
    int                         mCurrentPosition;  // 媒体当前播放的时间
    int                         mSeekPosition; // 需要查找的播放时间
    bool                        mPrepareSync; // 异步准备
    status_t                    mPrepareStatus; //  准备状态
    int                         mStreamType; // 流类型
    bool                        mLoop; // 是否循环
    float                       mLeftVolume; // 左声道音量
    float                       mRightVolume; // 右声道音量
    int                         mVideoWidth; // 视频宽度
    int                         mVideoHeight; // 视频高度
};

#endif // FFMPEG_MEDIAPLAYER_H
