package com.nicholasthorn.mc2;

import android.content.Context;
import android.net.wifi.WifiManager;
import android.os.Bundle;

import org.libsdl.app.SDLActivity;

/**
 * MechCommander 2 test build for Meta Quest: SDL's activity, loading
 * SDL2_mixer as well. The game itself is libmain.so (SDL_main).
 */
public class MC2Activity extends SDLActivity {
    private WifiManager.MulticastLock lanLock;

    @Override
    protected String[] getLibraries() {
        return new String[] { "SDL2", "SDL2_mixer", "main" };
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // Without this a headset's Wi-Fi drops broadcasts, and a game on the
        // local network (code/netlan.h) is never asked for, or never heard.
        WifiManager wifi = (WifiManager) getApplicationContext().getSystemService(Context.WIFI_SERVICE);
        if (wifi != null) {
            lanLock = wifi.createMulticastLock("mc2-lan");
            lanLock.setReferenceCounted(false);
            lanLock.acquire();
        }
    }

    @Override
    protected void onDestroy() {
        if (lanLock != null) {
            lanLock.release();
            lanLock = null;
        }
        super.onDestroy();
    }
}
