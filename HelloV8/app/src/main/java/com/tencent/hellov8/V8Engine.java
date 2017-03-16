package com.tencent.hellov8;

import android.content.Context;

/**
 * Created by dsotsen on 16/9/2.
 */
public class V8Engine {
    public static native void init(Context context);
    public static native void execute(String script);
}
