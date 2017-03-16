package com.tencent.circularcamera;

import android.graphics.Point;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.widget.ImageButton;

import com.tencent.circularcamera.util.DisplayUtil;

public class MainCameraActivity extends AppCompatActivity {

    CircularCameraSurfaceView glSurfaceView = null;
    ImageButton shutterBtn;
    float previewRate = -1f;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_main_camera);
        glSurfaceView = (CircularCameraSurfaceView)findViewById(R.id.camera_textureview);
        shutterBtn = (ImageButton)findViewById(R.id.btn_shutter);
        initViewParams();
        shutterBtn.setOnClickListener(new BtnListeners());
    }
    private void initViewParams(){
        ViewGroup.LayoutParams params = glSurfaceView.getLayoutParams();
        Point p = DisplayUtil.getScreenMetrics(this);
        params.width = 500;
        params.height = 500;
        previewRate = DisplayUtil.getScreenRate(this);
        glSurfaceView.setLayoutParams(params);
        ViewGroup.LayoutParams p2 = shutterBtn.getLayoutParams();
        p2.width = DisplayUtil.dip2px(this, 80);
        p2.height = DisplayUtil.dip2px(this, 80);;
        shutterBtn.setLayoutParams(p2);

    }

    private class BtnListeners implements View.OnClickListener {

        @Override
        public void onClick(View v) {
            switch(v.getId()){
                case R.id.btn_shutter:
                    CameraInterface.getInstance().doTakePicture();
                    break;
                default:break;
            }
        }

    }
    @Override
    protected void onResume() {
        super.onResume();
        glSurfaceView.bringToFront();
    }

    @Override
    protected void onPause() {
        super.onPause();
        glSurfaceView.onPause();
    }

}
