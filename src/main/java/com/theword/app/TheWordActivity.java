package com.theword.app;

import android.os.Bundle;
import android.app.NativeActivity;
import android.view.inputmethod.InputMethodManager;
import android.view.Window;

public class TheWordActivity extends NativeActivity {
    static {
        System.loadLibrary("theword");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    public void showKeyboard() {
        Window w = getWindow();
        if (w == null) return;
        InputMethodManager imm = (InputMethodManager) getSystemService(INPUT_METHOD_SERVICE);
        if (imm != null) {
            imm.showSoftInput(w.getDecorView(), InputMethodManager.SHOW_IMPLICIT);
        }
    }

    public void hideKeyboard() {
        Window w = getWindow();
        if (w == null) return;
        InputMethodManager imm = (InputMethodManager) getSystemService(INPUT_METHOD_SERVICE);
        if (imm != null) {
            imm.hideSoftInputFromWindow(w.getDecorView().getWindowToken(), 0);
        }
    }
}
