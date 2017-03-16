package com.tencent.helloluajit;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.View;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity {

    private TextView scriptView;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        scriptView = (TextView)findViewById(R.id.script);
        findViewById(R.id.btn_evaluate).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String scripts = scriptView.getText().toString();
                if(!TextUtils.isEmpty(scripts)) {
                    eval(scripts);
                }
            }
        });
    }

    static {
        System.loadLibrary("luajit");
        System.loadLibrary("helloluajit");
    }

    public static native void makeTest(BenchListener listener);
    public static native void eval(String script);
}
