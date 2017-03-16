package com.tencent.hellov8;

import android.content.res.AssetManager;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.EditText;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity {
    EditText text;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        V8Engine.init(getApplicationContext());
        setContentView(R.layout.activity_main);
        text = (EditText)findViewById(R.id.scripts);
        findViewById(R.id.hello).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                makeTest(new BenchListener() {
                    @Override
                    public void onResult(String res) {
                        TextView tv = (TextView)findViewById(R.id.hello);
                        tv.setText(res);
                        V8Engine.execute(text.getText().toString());
                    }
                });
            }
        });
    }

    static {
        System.loadLibrary("hellov8");
    }

    public static native void makeTest(BenchListener listener);
}
