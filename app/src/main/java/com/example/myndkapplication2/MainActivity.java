package com.example.myndkapplication2;

import androidx.appcompat.app.AppCompatActivity;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.TextView;
import com.example.myndkapplication2.databinding.ActivityMainBinding;

import java.io.File;


/**
 * 1）播放器是一款逐帧加密的混合式视频加密播放器，用于保护视频版权；需要android 9.0以上系统，或者鸿蒙系统；
 *
 * （2）、安卓手机要关闭开发者模式、关闭USB调试、播放过程中禁止手机连接电脑；
 *
 * （3）、安卓播放器禁止：虚拟机录屏、软件录屏、投屏录屏、外接采集卡录屏、root环境、模拟器环境；
 *
 * （5）、本播放器只是播放并保护视频不被翻录破解，视频所有权不为本播放器所有，也不追溯视频来源，由此发生的视频版权问题和播放
 */
public class MainActivity extends AppCompatActivity {

    private ActivityMainBinding binding;
    private SurfaceView surfaceView;
    private SurfaceHolder surfaceHolder;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());
        surfaceView = findViewById(R.id.surfaceView);
        surfaceHolder = surfaceView.getHolder();

        // Example of a call to a native method
//        TextView tv = binding.sampleText;
//        tv.setText(stringFromJNI());
        binding.btnPlay.setOnClickListener(view -> {
//            String videoPath = getExternalCacheDir() + "/test4.mp3";
            String videoPath = getExternalCacheDir() + "/test4.mp4";
            Log.d(">>>>", videoPath);
            File file = new File(videoPath);
            if (file.exists()) {
                Log.d(">>>>", "exist");
            } else {
                Log.d(">>>>", "no exist");
            }
            playVideo(videoPath, surfaceHolder.getSurface());
        });

        binding.btnPlayAudio.setOnClickListener(view -> {
            AudioPlayer audioPlayer = new AudioPlayer();
            String videoPath = getExternalCacheDir() + "/test4.mp4";
            audioPlayer.playAudio(videoPath);
        });

        binding.btnThreadTest.setOnClickListener(v -> {
            MyCThread cThread = new MyCThread();
            cThread.testCThread();
        });

        binding.btnThreadTest2.setOnClickListener(v -> {
//            MyCThread cThread = new MyCThread();
//            cThread.testCThread2();

            new Thread(new Runnable() {
                @Override
                public void run() {
                    int i = 0;
                    int j = i + 2;
                    Log.d(">>>>", "j = " + j);
                }
            }).start();
        });
    }

    /**
     * A native method that is implemented by the 'myndkapplication2' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();
    public native void playVideo(String path, Surface surface);

    // Used to load the 'myndkapplication2' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }
}