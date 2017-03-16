package com.tencent.helloneon;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        findViewById(R.id.hello).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                makeTest(new BenchListener() {
                    @Override
                    public void onResult(String res) {
                        TextView tv = (TextView)findViewById(R.id.hello);
                        tv.setText(res);
                    }
                });
            }
        });
    }

    static {
        System.loadLibrary("neon");
    }

    public static native void makeTest(BenchListener listener);
}
