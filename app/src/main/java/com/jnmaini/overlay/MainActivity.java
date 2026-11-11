package com.jnmaini.overlay;

import android.Manifest;
import android.app.Activity;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.provider.Settings;
import android.widget.Button;
import android.widget.Toast;

public class MainActivity extends Activity {
    private static final int REQ_NOTIF = 1001;
    private boolean pendingStart = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        Button overlay = findViewById(R.id.btnOverlay);
        Button start = findViewById(R.id.btnStart);
        Button game = findViewById(R.id.btnGame);
        overlay.setOnClickListener(v -> openOverlaySettings());
        start.setOnClickListener(v -> tryStart());
        game.setOnClickListener(v -> openCodm());
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (!canOverlay()) return;
        if (Build.VERSION.SDK_INT >= 33
                && checkSelfPermission(Manifest.permission.POST_NOTIFICATIONS) != PackageManager.PERMISSION_GRANTED
                && !pendingStart) {
            return;
        }
        tryStart();
    }

    private void openOverlaySettings() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) return;
        pendingStart = true;
        startActivity(new Intent(
                Settings.ACTION_MANAGE_OVERLAY_PERMISSION,
                Uri.parse("package:" + getPackageName())));
    }

    private void tryStart() {
        if (!canOverlay()) {
            openOverlaySettings();
            Toast.makeText(this, R.string.toast_need_overlay, Toast.LENGTH_LONG).show();
            return;
        }
        if (Build.VERSION.SDK_INT >= 33
                && checkSelfPermission(Manifest.permission.POST_NOTIFICATIONS) != PackageManager.PERMISSION_GRANTED) {
            pendingStart = true;
            requestPermissions(new String[]{Manifest.permission.POST_NOTIFICATIONS}, REQ_NOTIF);
            return;
        }
        startOverlay();
    }

    private void startOverlay() {
        if (OverlayService.instance != null) {
            Toast.makeText(this, R.string.overlay_running, Toast.LENGTH_SHORT).show();
            moveTaskToBack(true);
            return;
        }
        try {
            Intent svc = new Intent(this, OverlayService.class);
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) startForegroundService(svc);
            else startService(svc);
            Toast.makeText(this, R.string.toast_started, Toast.LENGTH_SHORT).show();
            moveTaskToBack(true);
        } catch (Throwable t) {
            Toast.makeText(this, t.getMessage() != null ? t.getMessage() : "Failed", Toast.LENGTH_LONG).show();
        }
    }

    private void openCodm() {
        try {
            Intent launch = getPackageManager().getLaunchIntentForPackage("com.garena.game.codm");
            if (launch == null) {
                Toast.makeText(this, "CODM is not installed", Toast.LENGTH_LONG).show();
                return;
            }
            launch.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            startActivity(launch);
        } catch (Throwable t) {
            Toast.makeText(this, "Cannot open CODM", Toast.LENGTH_SHORT).show();
        }
    }

    private boolean canOverlay() {
        return Build.VERSION.SDK_INT < Build.VERSION_CODES.M || Settings.canDrawOverlays(this);
    }

    @Override
    public void onRequestPermissionsResult(int code, String[] perms, int[] results) {
        super.onRequestPermissionsResult(code, perms, results);
        if (code == REQ_NOTIF && pendingStart) {
            pendingStart = false;
            tryStart();
        }
    }
}
