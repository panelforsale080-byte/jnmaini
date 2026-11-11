package com.jnmaini.overlay;

public final class NativeBridge {
    static {
        System.loadLibrary("ZAGll");
    }

    private NativeBridge() {}

    public static native void nativeSetDisplaySize(int w, int h);
    public static native void nativeGlDrawFrame();
    public static native void nativeTouch(int action, float x, float y);
    public static native void nativeSetMenuOpen(boolean open);
    public static native boolean nativeIsMenuOpen();
    public static native int nativePollAttach();
}
