package com.jnmaini.overlay;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Intent;
import android.graphics.PixelFormat;
import android.os.Build;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.widget.Toast;
import android.provider.Settings;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.TextView;

public class OverlayService extends Service {
    public static OverlayService instance;

    private static final String CHANNEL = "jnmaini_overlay";
    private static final int FLAGS_PASS =
            WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE
                    | WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE
                    | WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN
                    | WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS;
    private static final int FLAGS_MENU =
            WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE
                    | WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN
                    | WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS;

    private WindowManager wm;
    private GlMenuView glView;
    private TextView fab;
    private WindowManager.LayoutParams surfaceLp;
    private boolean menuOpen = true;
    private boolean readyToasted = false;
    private final Handler main = new Handler(Looper.getMainLooper());
    private final Runnable pollGame = new Runnable() {
        @Override
        public void run() {
            int state = 0;
            try {
                state = NativeBridge.nativePollAttach();
            } catch (Throwable ignored) {
            }
            if (state >= 1 && !readyToasted) {
                readyToasted = true;
                Toast.makeText(OverlayService.this, R.string.toast_resolved, Toast.LENGTH_LONG).show();
            } else if (state == 0) {
                readyToasted = false;
            }
            main.postDelayed(this, 1000L);
        }
    };

    @Override
    public void onCreate() {
        super.onCreate();
        instance = this;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && !Settings.canDrawOverlays(this)) {
            stopSelf();
            return;
        }
        startForegroundNotif();
        wm = (WindowManager) getSystemService(WINDOW_SERVICE);
        attachWindows();
        main.post(pollGame);
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        return START_STICKY;
    }

    @Override
    public void onDestroy() {
        if (wm != null && glView != null) {
            try { wm.removeView(glView); } catch (Throwable ignored) {}
        }
        if (wm != null && fab != null) {
            try { wm.removeView(fab); } catch (Throwable ignored) {}
        }
        glView = null;
        fab = null;
        main.removeCallbacks(pollGame);
        if (instance == this) instance = null;
        super.onDestroy();
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    private int dp(int v) {
        return (int) TypedValue.applyDimension(
                TypedValue.COMPLEX_UNIT_DIP, v, getResources().getDisplayMetrics());
    }

    private int overlayType() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            return WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY;
        }
        return WindowManager.LayoutParams.TYPE_PHONE;
    }

    private void startForegroundNotif() {
        NotificationManager nm = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            NotificationChannel channel = new NotificationChannel(
                    CHANNEL, getString(R.string.notif_channel), NotificationManager.IMPORTANCE_LOW);
            nm.createNotificationChannel(channel);
        }
        Notification.Builder builder = Build.VERSION.SDK_INT >= Build.VERSION_CODES.O
                ? new Notification.Builder(this, CHANNEL)
                : new Notification.Builder(this);
        Notification notification = builder
                .setContentTitle(getString(R.string.app_name))
                .setContentText(getString(R.string.notif_text))
                .setSmallIcon(android.R.drawable.ic_menu_view)
                .setOngoing(true)
                .build();
        if (Build.VERSION.SDK_INT >= 34) {
            startForeground(1, notification, android.content.pm.ServiceInfo.FOREGROUND_SERVICE_TYPE_SPECIAL_USE);
        } else {
            startForeground(1, notification);
        }
    }

    private void attachWindows() {
        glView = new GlMenuView(this);
        glView.setMenuStateListener(open -> glView.post(() -> setMenuTouchable(open)));
        surfaceLp = new WindowManager.LayoutParams(
                WindowManager.LayoutParams.MATCH_PARENT,
                WindowManager.LayoutParams.MATCH_PARENT,
                overlayType(),
                FLAGS_MENU,
                PixelFormat.TRANSLUCENT);
        surfaceLp.gravity = Gravity.TOP | Gravity.START;
        wm.addView(glView, surfaceLp);

        fab = new TextView(this);
        fab.setText("JN");
        fab.setTextColor(0xFFFFFFFF);
        fab.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        fab.setGravity(Gravity.CENTER);
        fab.setBackgroundResource(R.drawable.bg_fab);
        int size = dp(56);
        fab.setOnClickListener(v -> {
            boolean open = !NativeBridge.nativeIsMenuOpen();
            NativeBridge.nativeSetMenuOpen(open);
            setMenuTouchable(open);
        });
        WindowManager.LayoutParams fabLp = new WindowManager.LayoutParams(
                size, size, overlayType(),
                WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE | WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN,
                PixelFormat.TRANSLUCENT);
        fabLp.gravity = Gravity.TOP | Gravity.START;
        fabLp.x = dp(16);
        fabLp.y = dp(48);
        wm.addView(fab, fabLp);
    }

    private void setMenuTouchable(boolean open) {
        if (wm == null || glView == null || surfaceLp == null || menuOpen == open) return;
        menuOpen = open;
        surfaceLp.flags = open ? FLAGS_MENU : FLAGS_PASS;
        try {
            wm.updateViewLayout(glView, surfaceLp);
        } catch (Throwable ignored) {}
    }
}
