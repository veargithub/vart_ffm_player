package com.example.myndkapplication2;

public class MyCThread {

    static {
        System.loadLibrary("native-lib");
    }

    public native void testCThread();

    public native void testCThread2();
}
