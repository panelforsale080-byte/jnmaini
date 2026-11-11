package com.jnmaini.overlay;

import android.content.Context;
import android.graphics.PixelFormat;
import android.opengl.GLSurfaceView;
import android.view.MotionEvent;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class GlMenuView extends GLSurfaceView implements GLSurfaceView.Renderer {
    public interface MenuStateListener {
        void onMenuState(boolean open);
    }

    private MenuStateListener menuStateListener;
    private boolean lastOpen = true;

    public GlMenuView(Context context) {
        super(context);
        setEGLContextClientVersion(3);
        setEGLConfigChooser(8, 8, 8, 8, 16, 0);
        getHolder().setFormat(PixelFormat.TRANSLUCENT);
        setZOrderOnTop(true);
        setPreserveEGLContextOnPause(true);
        setRenderer(this);
        setRenderMode(RENDERMODE_CONTINUOUSLY);
    }

    public void setMenuStateListener(MenuStateListener listener) {
        menuStateListener = listener;
    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        NativeBridge.nativeSetMenuOpen(true);
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        NativeBridge.nativeSetDisplaySize(width, height);
    }

    @Override
    public void onDrawFrame(GL10 gl) {
        NativeBridge.nativeGlDrawFrame();
        boolean open = NativeBridge.nativeIsMenuOpen();
        if (open != lastOpen) {
            lastOpen = open;
            if (menuStateListener != null) menuStateListener.onMenuState(open);
        }
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        NativeBridge.nativeTouch(event.getActionMasked(), event.getX(), event.getY());
        return true;
    }
}
