package com.tencent.lua;

/**
 * LuaEngine by alexqzhou
 */
public class Engine
{
    public Engine() {

    }


    private long mStateInstance;

    static {
        System.load("luajit");
    }

    public native String eval(String script);

    private native void init();

    public static native String getVersion();
}