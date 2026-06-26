package com.theword.app;

import android.os.Bundle;
import android.app.NativeActivity;

public class TheWordActivity extends NativeActivity {
    static {
        System.loadLibrary("theword");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }
}
