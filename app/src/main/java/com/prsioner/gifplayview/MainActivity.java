package com.prsioner.gifplayview;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;

import android.app.Activity;
import android.content.pm.PackageManager;
import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;

import com.bumptech.glide.Glide;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'native-lib' library on application startup.

    private static final String TAG = MainActivity.class.getSimpleName();

    ImageView gifView;
    Button glideLoadBtn;
    Button jniLoadBtn;

    private GifHelper gifHelper;

    private String gifFilePath;
    Bitmap gifBitmap;

    //文件名为assets内gif的文件名
    private String showGifName = "gif3.gif";
    private String assetGifPath = "file:///android_asset/"+showGifName;

    private Handler mHandler = new Handler(){
        public void handleMessage(Message msg) {
            int delay=gifHelper.updateFrame(gifBitmap);
            mHandler.sendEmptyMessageDelayed(1,delay);
            gifView.setImageBitmap(gifBitmap);
        }
    };
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        gifView = findViewById(R.id.imv_gif_play);
        glideLoadBtn = findViewById(R.id.btn_load_gif_by_glide);
        jniLoadBtn = findViewById(R.id.btn_load_gif_by_jni);

        glideLoadBtn.setOnClickListener(v -> {
            Glide.with(MainActivity.this)
            .asGif()
                    .load(assetGifPath)
                    .skipMemoryCache(false)
            .into(gifView);
        });

        jniLoadBtn.setOnClickListener(v -> {

            if(TextUtils.isEmpty(gifFilePath)) return;

            String url = gifFilePath;
            Log.d(TAG,"gif file path:"+url);

            gifHelper = GifHelper.getInstance(url);

            //获取图片宽高
            int width = gifHelper.getWidth();
            int height = gifHelper.getHeight();

            Log.d(TAG,"width:"+width);
            Log.d(TAG,"height:"+height);

            gifBitmap = Bitmap.createBitmap(width,height, Bitmap.Config.ARGB_8888);
            int delay = gifHelper.updateFrame(gifBitmap);
            gifView.setImageBitmap(gifBitmap);
            mHandler.sendEmptyMessageDelayed(1, delay);

        });

        PermissionsUtils.verifyStoragePermissions(this);
        assetsGifFileCopy();
    }


    /**
     * 测试用的文件无法直接在assets文件夹内使用需要拷贝到应用外部存储
     *
     */
    private void assetsGifFileCopy() {
        new Thread(() -> {

            FileUtils fileUtils = new FileUtils(MainActivity.this);
            gifFilePath = fileUtils.copyAssetAndWrite(showGifName);
        }).start();


    }



}