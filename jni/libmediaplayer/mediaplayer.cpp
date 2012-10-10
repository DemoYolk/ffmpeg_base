/*
 * mediaplayer.cpp
 */

//#define LOG_NDEBUG 0
#define TAG "FFMpegMediaPlayer"

#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

extern "C" {
	
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/log.h"
	
} // end of extern C

#include <android/log.h>

#include "mediaplayer.h"
#include "output.h"

#define FPS_DEBUGGING false

static MediaPlayer* sPlayer;

MediaPlayer::MediaPlayer()
{
    mListener = NULL;
    mCookie = NULL;
    mDuration = -1;
    mStreamType = MUSIC; //流类型为 音乐类型（暂时不懂有什么区别）
    mCurrentPosition = -1;
    mSeekPosition = -1;
    mCurrentState = MEDIA_PLAYER_IDLE; //默认为空闲状态
    mPrepareSync = false;
    mPrepareStatus = NO_ERROR;
    mLoop = false;
    //pthread_mutex_init();
    //以动态方式创建互斥锁的，参数attr指定了新建互斥锁的属性。
    //如果参数attr为空，则使用默认的互斥锁属性，默认属性为快速互斥锁 。
    //互斥锁的属性在创建锁的时候指定，在LinuxThreads实现中仅有一个锁类型属性，不同的锁类型在试图对一个已经被锁定的互斥锁加锁时表现不同。
    pthread_mutex_init(&mLock, NULL);
    mLeftVolume = mRightVolume = 1.0; //声音为1.0
    mVideoWidth = mVideoHeight = 0; //高度宽度初始化 为0
    sPlayer = this; //将自己付给指向自己的指针
}

MediaPlayer::~MediaPlayer()
{
	if(mListener != NULL) {
		free(mListener); //释放 播放器监听器 对象
	}
}

status_t MediaPlayer::prepareAudio()
{
	__android_log_print(ANDROID_LOG_INFO, TAG, "prepareAudio");
	mAudioStreamIndex = -1; //音频流索引
	for (int i = 0; i < mMovieFile->nb_streams; i++) {
		//遍历 媒体文件信息里包含的流，如果 找到了类型为音频的流 则 记录在文件流信息的 索引
		if (mMovieFile->streams[i]->codec->codec_type == CODEC_TYPE_AUDIO) {
			mAudioStreamIndex = i;
			break;
		}
	}
	
	if (mAudioStreamIndex == -1) {
		//没有找到音频流，返回不可用操作代码
		return INVALID_OPERATION;
	}

	// 找到了 ，将指针指向 音频流
	AVStream* stream = mMovieFile->streams[mAudioStreamIndex];
	// Get a pointer to the codec context for the video stream
	AVCodecContext* codec_ctx = stream->codec;
	//得到对应的解码器
	AVCodec* codec = avcodec_find_decoder(codec_ctx->codec_id);
	if (codec == NULL) {
		return INVALID_OPERATION;
	}
	
	// Open codec
	if (avcodec_open(codec_ctx, codec) < 0) {
		return INVALID_OPERATION;
	}

	// prepare os output 设置音频驱动：流类型，采样速率，16位宽，声道。返回0为成功
	if (Output::AudioDriver_set(MUSIC,
								stream->codec->sample_rate,
								PCM_16_BIT,
								(stream->codec->channels == 2) ? CHANNEL_OUT_STEREO
										: CHANNEL_OUT_MONO) != ANDROID_AUDIOTRACK_RESULT_SUCCESS) {
		return INVALID_OPERATION;
	}

	//开始音频
	if (Output::AudioDriver_start() != ANDROID_AUDIOTRACK_RESULT_SUCCESS) {
		return INVALID_OPERATION;
	}

	return NO_ERROR;
}

status_t MediaPlayer::prepareVideo()
{
	__android_log_print(ANDROID_LOG_INFO, TAG, "prepareVideo");
	// Find the first video stream
	mVideoStreamIndex = -1;
	//同上音频流寻找的步骤
	for (int i = 0; i < mMovieFile->nb_streams; i++) {
		if (mMovieFile->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO) {
			mVideoStreamIndex = i;
			break;
		}
	}
	
	if (mVideoStreamIndex == -1) {
		return INVALID_OPERATION;
	}
	
	AVStream* stream = mMovieFile->streams[mVideoStreamIndex];
	// Get a pointer to the codec context for the video stream
	AVCodecContext* codec_ctx = stream->codec;
	AVCodec* codec = avcodec_find_decoder(codec_ctx->codec_id);
	if (codec == NULL) {
		return INVALID_OPERATION;
	}
	
	// Open codec
	if (avcodec_open(codec_ctx, codec) < 0) {
		return INVALID_OPERATION;
	}
	
	mVideoWidth = codec_ctx->width;
	mVideoHeight = codec_ctx->height;
	mDuration =  mMovieFile->duration;
	//应该是转换图像的功能，返回一个上下文，用于转换图像。
	mConvertCtx = sws_getContext(stream->codec->width,
								 stream->codec->height,
								 stream->codec->pix_fmt,
								 stream->codec->width,
								 stream->codec->height,
								 PIX_FMT_RGB565,
								 SWS_POINT,
								 NULL,
								 NULL,
								 NULL);

	if (mConvertCtx == NULL) {
		return INVALID_OPERATION;
	}

	void*		pixels;
	//得到相应宽高的画布，并将pixels指向它
	if (Output::VideoDriver_getPixels(stream->codec->width,
									  stream->codec->height,
									  &pixels) != ANDROID_SURFACE_RESULT_SUCCESS) {
		return INVALID_OPERATION;
	}
	//初始化一个动态AVFrame
	mFrame = avcodec_alloc_frame();
	if (mFrame == NULL) {
		return INVALID_OPERATION;
	}
	// Assign appropriate parts of buffer to image planes in pFrameRGB
	// Note that pFrameRGB is an AVFrame, but AVFrame is a superset
	// of AVPicture（分配适当的缓冲区给图片平台）
	avpicture_fill((AVPicture *) mFrame,
				   (uint8_t *) pixels,
				   PIX_FMT_RGB565,
				   stream->codec->width,
				   stream->codec->height);

	return NO_ERROR;
}

status_t MediaPlayer::prepare()
{
	status_t ret;
	mCurrentState = MEDIA_PLAYER_PREPARING;
	av_log_set_callback(ffmpegNotify);//设置 ffmpeg的回调 信息 监听器
	if ((ret = prepareVideo()) != NO_ERROR) {
		mCurrentState = MEDIA_PLAYER_STATE_ERROR;
		return ret;
	}
	if ((ret = prepareAudio()) != NO_ERROR) {
		mCurrentState = MEDIA_PLAYER_STATE_ERROR;
		return ret;
	}
	mCurrentState = MEDIA_PLAYER_PREPARED;
	return NO_ERROR;
}

status_t MediaPlayer::setListener(MediaPlayerListener* listener)
{
    __android_log_print(ANDROID_LOG_INFO, TAG, "setListener");
    mListener = listener;
    return NO_ERROR;
}

status_t MediaPlayer::setDataSource(const char *url)
{
    __android_log_print(ANDROID_LOG_INFO, TAG, "setDataSource(%s)", url);
    status_t err = BAD_VALUE;
	// Open video file
	if(av_open_input_file(&mMovieFile, url, NULL, 0, NULL) != 0) {
		__android_log_print(ANDROID_LOG_INFO, TAG, "av_open_input_file != 0!");
		return INVALID_OPERATION;
	}
	// Retrieve stream information
	if(av_find_stream_info(mMovieFile) < 0) {
		__android_log_print(ANDROID_LOG_INFO, TAG, "av_find_stream_info < 0!");
		return INVALID_OPERATION;
	}
	mCurrentState = MEDIA_PLAYER_INITIALIZED;
    return NO_ERROR;
}

status_t MediaPlayer::suspend() {
	__android_log_print(ANDROID_LOG_INFO, TAG, "suspend");
	
	mCurrentState = MEDIA_PLAYER_STOPPED;
	if(mDecoderAudio != NULL) {
		mDecoderAudio->stop();
	}
	if(mDecoderVideo != NULL) {
		mDecoderVideo->stop();
	}
	//阻塞线程
	if(pthread_join(mPlayerThread, NULL) != 0) {
		__android_log_print(ANDROID_LOG_ERROR, TAG, "Couldn't cancel player thread");
	}
	
	// Close the codec
	free(mDecoderAudio);
	free(mDecoderVideo);
	
	// Close the video file
	av_close_input_file(mMovieFile);

	//close OS drivers
	Output::AudioDriver_unregister();
	Output::VideoDriver_unregister();

	__android_log_print(ANDROID_LOG_ERROR, TAG, "suspended");

    return NO_ERROR;
}

status_t MediaPlayer::resume() {
	//pthread_mutex_lock(&mLock);
	mCurrentState = MEDIA_PLAYER_STARTED;
	//pthread_mutex_unlock(&mLock);
    return NO_ERROR;
}

status_t MediaPlayer::setVideoSurface(JNIEnv* env, jobject jsurface)
{ 
	//注册 视频驱动
	if(Output::VideoDriver_register(env, jsurface) != ANDROID_SURFACE_RESULT_SUCCESS) {
		return INVALID_OPERATION;
	}
	//注册 音频驱动
	if(Output::AudioDriver_register() != ANDROID_AUDIOTRACK_RESULT_SUCCESS) {
		return INVALID_OPERATION;
	}	
    return NO_ERROR;
}
/**
 * 是否取消？有什么用，看不懂
 */
bool MediaPlayer::shouldCancel(PacketQueue* queue)
{
	return (mCurrentState == MEDIA_PLAYER_STATE_ERROR || mCurrentState == MEDIA_PLAYER_STOPPED ||
			 ((mCurrentState == MEDIA_PLAYER_DECODED || mCurrentState == MEDIA_PLAYER_STARTED) 
			  && queue->size() == 0));
}

void MediaPlayer::decode(AVFrame* frame, double pts)
{
	//我靠，这个是它用来调试用的？
	if(FPS_DEBUGGING) {
		timeval pTime;
		static int frames = 0;
		static double t1 = -1;
		static double t2 = -1;

		gettimeofday(&pTime, NULL);
		t2 = pTime.tv_sec + (pTime.tv_usec / 1000000.0);
		if (t1 == -1 || t2 > t1 + 1) {
			__android_log_print(ANDROID_LOG_INFO, TAG, "Video fps:%i", frames);
			//sPlayer->notify(MEDIA_INFO_FRAMERATE_VIDEO, frames, -1);
			t1 = t2;
			frames = 0;
		}
		frames++;
	}

	// Convert the image from its native format to RGB
	sws_scale(sPlayer->mConvertCtx,
		      frame->data,
		      frame->linesize,
			  0,
			  sPlayer->mVideoHeight,
			  sPlayer->mFrame->data,
			  sPlayer->mFrame->linesize);
	//更新界面
	Output::VideoDriver_updateSurface();
}

void MediaPlayer::decodeAudio(int16_t* buffer, int buffer_size)
{
	if(FPS_DEBUGGING) {
		timeval pTime;
		static int frames = 0;
		static double t1 = -1;
		static double t2 = -1;

		gettimeofday(&pTime, NULL);
		t2 = pTime.tv_sec + (pTime.tv_usec / 1000000.0);
		if (t1 == -1 || t2 > t1 + 1) {
			__android_log_print(ANDROID_LOG_INFO, TAG, "Audio fps:%i", frames);
			//sPlayer->notify(MEDIA_INFO_FRAMERATE_AUDIO, frames, -1);
			t1 = t2;
			frames = 0;
		}
		frames++;
	}

	//将数据写入本地音频轨道
	if(Output::AudioDriver_write(buffer, buffer_size) <= 0) {
		__android_log_print(ANDROID_LOG_ERROR, TAG, "Couldn't write samples to audio track");
	}
}

void MediaPlayer::decodeMovie(void* ptr)
{
	AVPacket pPacket;
	
	AVStream* stream_audio = mMovieFile->streams[mAudioStreamIndex];
	mDecoderAudio = new DecoderAudio(stream_audio);
	mDecoderAudio->onDecode = decodeAudio;
	mDecoderAudio->startAsync();
	
	AVStream* stream_video = mMovieFile->streams[mVideoStreamIndex];
	mDecoderVideo = new DecoderVideo(stream_video);
	mDecoderVideo->onDecode = decode;
	mDecoderVideo->startAsync();
	
	mCurrentState = MEDIA_PLAYER_STARTED;
	__android_log_print(ANDROID_LOG_INFO, TAG, "playing %ix%i", mVideoWidth, mVideoHeight);
	//这个线程是在取出文件中的视频包和音频包并将其放入到 对因解码器的解码包队列中，直到程序暂停或解码完毕或解码错误
	while (mCurrentState != MEDIA_PLAYER_DECODED && mCurrentState != MEDIA_PLAYER_STOPPED &&
		   mCurrentState != MEDIA_PLAYER_STATE_ERROR)
	{
		//如果 解码包队列大约了 规定的 大小，暂停200毫秒 等待解码线程，再继续 向其中添加数据
		if (mDecoderVideo->packets() > FFMPEG_PLAYER_MAX_QUEUE_SIZE &&
				mDecoderAudio->packets() > FFMPEG_PLAYER_MAX_QUEUE_SIZE) {
			usleep(200);
			continue;
		}
		
		if(av_read_frame(mMovieFile, &pPacket) < 0) {
			//如果在文件中不能读出完整的一帧，则认为是解码完毕
			mCurrentState = MEDIA_PLAYER_DECODED;
			continue;
		}
		
		// Is this a packet from the video stream?
		if (pPacket.stream_index == mVideoStreamIndex) {
			mDecoderVideo->enqueue(&pPacket);
		} 
		else if (pPacket.stream_index == mAudioStreamIndex) {
			mDecoderAudio->enqueue(&pPacket);
		}
		else {
			// Free the packet that was allocated by av_read_frame
			av_free_packet(&pPacket);
		}
	}
	
	//waits on end of video thread
	__android_log_print(ANDROID_LOG_ERROR, TAG, "waiting on video thread");
	int ret = -1;
	//阻塞等待视频解码结束
	if((ret = mDecoderVideo->wait()) != 0) {
		__android_log_print(ANDROID_LOG_ERROR, TAG, "Couldn't cancel video thread: %i", ret);
	}
	
	__android_log_print(ANDROID_LOG_ERROR, TAG, "waiting on audio thread");
	//阻塞等待音频解码结束
	if((ret = mDecoderAudio->wait()) != 0) {
		__android_log_print(ANDROID_LOG_ERROR, TAG, "Couldn't cancel audio thread: %i", ret);
	}
    
	if(mCurrentState == MEDIA_PLAYER_STATE_ERROR) {
		__android_log_print(ANDROID_LOG_INFO, TAG, "playing err");
	}
	mCurrentState = MEDIA_PLAYER_PLAYBACK_COMPLETE;
	__android_log_print(ANDROID_LOG_INFO, TAG, "end of playing");
}

void* MediaPlayer::startPlayer(void* ptr)
{
    __android_log_print(ANDROID_LOG_INFO, TAG, "starting main player thread");
    sPlayer->decodeMovie(ptr);
}

status_t MediaPlayer::start()
{
	if (mCurrentState != MEDIA_PLAYER_PREPARED) {
		return INVALID_OPERATION;
	}
	pthread_create(&mPlayerThread, NULL, startPlayer, NULL);
	return NO_ERROR;
}

status_t MediaPlayer::stop()
{
	//pthread_mutex_lock(&mLock);
	mCurrentState = MEDIA_PLAYER_STOPPED; //改变播放状态就能在解码线程里控制解码的状态
	//pthread_mutex_unlock(&mLock);
    return NO_ERROR;
}

status_t MediaPlayer::pause()
{
	//pthread_mutex_lock(&mLock);
	mCurrentState = MEDIA_PLAYER_PAUSED;
	//pthread_mutex_unlock(&mLock);
	return NO_ERROR;
}

bool MediaPlayer::isPlaying()
{
    return mCurrentState == MEDIA_PLAYER_STARTED || 
		mCurrentState == MEDIA_PLAYER_DECODED;
}

status_t MediaPlayer::getVideoWidth(int *w)
{
	if (mCurrentState < MEDIA_PLAYER_PREPARED) {
		return INVALID_OPERATION;
	}
	*w = mVideoWidth;
    return NO_ERROR;
}

status_t MediaPlayer::getVideoHeight(int *h)
{
	if (mCurrentState < MEDIA_PLAYER_PREPARED) {
		return INVALID_OPERATION;
	}
	*h = mVideoHeight;
    return NO_ERROR;
}

status_t MediaPlayer::getCurrentPosition(int *msec)
{
	if (mCurrentState < MEDIA_PLAYER_PREPARED) {
		return INVALID_OPERATION;
	}
	*msec = 0/*av_gettime()*/;//TODO
	//__android_log_print(ANDROID_LOG_INFO, TAG, "position %i", *msec);
	return NO_ERROR;
}

status_t MediaPlayer::getDuration(int *msec)
{
	if (mCurrentState < MEDIA_PLAYER_PREPARED) {
		return INVALID_OPERATION;
	}
	*msec = mDuration / 1000;
    return NO_ERROR;
}

status_t MediaPlayer::seekTo(int msec)
{
    return INVALID_OPERATION; //TODO
}

status_t MediaPlayer::reset()
{
    return INVALID_OPERATION; //TODO
}

status_t MediaPlayer::setAudioStreamType(int type)
{
	return NO_ERROR; //TODO
}
/**
 * ffmpeg的通知
 */
void MediaPlayer::ffmpegNotify(void* ptr, int level, const char* fmt, va_list vl) {
	
	switch(level) {
			/**
			 * Something went really wrong and we will crash now.
			 */
		case AV_LOG_PANIC:
			__android_log_print(ANDROID_LOG_ERROR, TAG, "AV_LOG_PANIC: %s", fmt);
			//sPlayer->mCurrentState = MEDIA_PLAYER_STATE_ERROR;
			break;
			
			/**
			 * Something went wrong and recovery is not possible.
			 * For example, no header was found for a format which depends
			 * on headers or an illegal combination of parameters is used.
			 */
		case AV_LOG_FATAL:
			__android_log_print(ANDROID_LOG_ERROR, TAG, "AV_LOG_FATAL: %s", fmt);
			//sPlayer->mCurrentState = MEDIA_PLAYER_STATE_ERROR;
			break;
			
			/**
			 * Something went wrong and cannot losslessly be recovered.
			 * However, not all future data is affected.
			 */
		case AV_LOG_ERROR:
			__android_log_print(ANDROID_LOG_ERROR, TAG, "AV_LOG_ERROR: %s", fmt);
			//sPlayer->mCurrentState = MEDIA_PLAYER_STATE_ERROR;
			break;
			
			/**
			 * Something somehow does not look correct. This may or may not
			 * lead to problems. An example would be the use of '-vstrict -2'.
			 */
		case AV_LOG_WARNING:
			__android_log_print(ANDROID_LOG_ERROR, TAG, "AV_LOG_WARNING: %s", fmt);
			break;
			
		case AV_LOG_INFO:
			__android_log_print(ANDROID_LOG_INFO, TAG, "%s", fmt);
			break;
			
		case AV_LOG_DEBUG:
			__android_log_print(ANDROID_LOG_DEBUG, TAG, "%s", fmt);
			break;
			
	}
}
/**
 * 通知
 */
void MediaPlayer::notify(int msg, int ext1, int ext2)
{
    //__android_log_print(ANDROID_LOG_INFO, TAG, "message received msg=%d, ext1=%d, ext2=%d", msg, ext1, ext2);
    bool send = true;
    bool locked = false;

    if ((mListener != 0) && send) {
       //__android_log_print(ANDROID_LOG_INFO, TAG, "callback application");
       mListener->notify(msg, ext1, ext2);
       //__android_log_print(ANDROID_LOG_INFO, TAG, "back from callback");
	}
}
