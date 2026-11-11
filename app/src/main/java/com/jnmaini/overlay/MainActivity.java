package com.jnmaini.overlay;

import android.app.Activity;
import android.os.Bundle;
import android.widget.TextView;

public class MainActivity extends Activity {
    static {
        System.loadLibrary("ZAGll");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        TextView view = new TextView(this);
        view.setText("JNMaini");
        view.setPadding(48, 48, 48, 48);
        setContentView(view);
    }
}
