package com.media.ffmpeg.android;

import java.io.IOException;

import android.content.Context;
import android.util.AttributeSet;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.MediaController;
import android.widget.MediaController.MediaPlayerControl;

import com.media.ffmpeg.FFMpegPlayer;

/**
 * 视频显示界面
 * 
 * @author demo
 * 
 */
public class FFMpegMovieViewAndroid extends SurfaceView {
	private static final String TAG = "FFMpegMovieViewAndroid";

	/** ffmpeg的播放器 */
	private FFMpegPlayer mPlayer;
	/** 视频控制器 */
	private MediaController mMediaController; // 使用的是 android自带的多媒体控制器

	public FFMpegMovieViewAndroid(Context context) {
		super(context);
		initVideoView();
	}

	public FFMpegMovieViewAndroid(Context context, AttributeSet attrs) {
		this(context, attrs, 0);
		initVideoView();
	}

	public FFMpegMovieViewAndroid(Context context, AttributeSet attrs,
			int defStyle) {
		super(context, attrs, defStyle);
		initVideoView();
	}

	/**
	 * 初始化播放器和surfaceHolder
	 */
	private void initVideoView() {
		mPlayer = new FFMpegPlayer();

		SurfaceHolder surfHolder = getHolder();
		surfHolder.addCallback(mSHCallback);
	}

	/**
	 * 将 多媒体控制器 依附到这个显示界面
	 */
	private void attachMediaController() {
		mMediaController = new MediaController(getContext());
		View anchorView = this.getParent() instanceof View ? (View) this
				.getParent() : this;
		mMediaController.setMediaPlayer(mMediaPlayerControl);
		mMediaController.setAnchorView(anchorView);
		mMediaController.setEnabled(true);
	}

	/**
	 * 设置视频路径
	 * 
	 * @param filePath
	 * @throws IllegalArgumentException
	 * @throws IllegalStateException
	 * @throws IOException
	 */
	public void setVideoPath(String filePath) throws IllegalArgumentException,
			IllegalStateException, IOException {
		mPlayer.setDataSource(filePath);
		// mPlayer.setDataSource("http://img.pems.cn/interpose/decompression/file/20/00/01/10/200001108/intervention_address_200001108.flv");
		Log.i(TAG, "mPlayer is setDataSourced!");
	}

	/**
	 * initzialize player
	 */
	private void openVideo(SurfaceHolder surfHolder) {
		try {
			mPlayer.setDisplay(surfHolder);
			Log.i(TAG, "mPlayer is prepared!");
			mPlayer.prepare();
		} catch (IllegalStateException e) {
			Log.e(TAG, "Couldn't prepare player: " + e.getMessage());
		} catch (IOException e) {
			Log.e(TAG, "Couldn't prepare player: " + e.getMessage());
		}
	}

	/**
	 * 开始播放视频
	 */
	private void startVideo() {

		attachMediaController();

		mPlayer.start();
		Log.i(TAG, "mPlayer is started!");
	}

	private void release() {
		Log.d(TAG, "releasing player");

		mPlayer.suspend();
		Log.i(TAG, "mPlayer is suspend!");

		Log.d(TAG, "released");
	}

	public boolean onTouchEvent(android.view.MotionEvent event) {
		if (!mMediaController.isShowing()) {
			mMediaController.show(3000);
		}
		return true;
	}

	SurfaceHolder.Callback mSHCallback = new SurfaceHolder.Callback() {
		public void surfaceChanged(SurfaceHolder holder, int format, int w,
				int h) {
			Log.i(TAG, "surfaceView is changed!");
			// 开始播放视频
			startVideo();
		}

		public void surfaceCreated(SurfaceHolder holder) {
			Log.i(TAG, "surfaceView is created!");
			// 开启一个视频
			openVideo(holder);
		}

		public void surfaceDestroyed(SurfaceHolder holder) {
			Log.i(TAG, "surfaceView is destroyed!");
			release();
			if (mMediaController.isShowing()) {
				mMediaController.hide();
			}
		}
	};

	MediaPlayerControl mMediaPlayerControl = new MediaPlayerControl() {

		public void start() {
			mPlayer.resume();
		}

		public void seekTo(int pos) {
			Log.d(TAG, "want seek to");
		}

		public void pause() {
			mPlayer.pause();
		}

		public boolean isPlaying() {
			return mPlayer.isPlaying();
		}

		public int getDuration() {
			return mPlayer.getDuration();
		}

		public int getCurrentPosition() {
			return mPlayer.getCurrentPosition();
		}

		public int getBufferPercentage() {
			Log.d(TAG, "want buffer percentage");
			return 0;
		}

		@Override
		public boolean canPause() {
			// TODO Auto-generated method stub
			return false;
		}

		@Override
		public boolean canSeekBackward() {
			// TODO Auto-generated method stub
			return false;
		}

		@Override
		public boolean canSeekForward() {
			// TODO Auto-generated method stub
			return false;
		}
	};
}
