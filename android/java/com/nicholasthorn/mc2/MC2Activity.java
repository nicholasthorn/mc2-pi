package com.nicholasthorn.mc2;

import org.libsdl.app.SDLActivity;

/**
 * MechCommander 2 test build for Meta Quest: SDL's activity, loading
 * SDL2_mixer as well. The game itself is libmain.so (SDL_main).
 */
public class MC2Activity extends SDLActivity {
    @Override
    protected String[] getLibraries() {
        return new String[] { "SDL2", "SDL2_mixer", "main" };
    }
}
