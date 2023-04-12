#include "lib-internal.h"

/*
Ideas:
- GIMP can output C header, raw binary, etc files, make that the sprite format
*/

DisplayConfig_t * displayConfig;
ControllerConfig_t * controllerConfig;
AudioConfig_t * audioConfig;
SDConfig_t * sdConfig;

static void drawLogo();

int initPicoVGA(DisplayConfig_t * displayConf, ControllerConfig_t * controllerConf, AudioConfig_t * audioConf,
                SDConfig_t * sdConf) {
    if(displayConf != NULL) {
        displayConfig = displayConf;
        if(initDisplay()) return 1; //initDisplay.c
    }
    if(controllerConf != NULL) {
        controllerConfig = controllerConf;
        if(initController()) return 1; //controller.c
    }
    if(audioConf != NULL) {
        audioConfig = audioConf;
        if(initAudio()) return 1; //audio.c
    }
    if(sdConf != NULL) {
        sdConfig = sdConf;
        if(initSD()) return 1; //sd.c
    }
    
    busyWait(10000000); //wait to make sure everything is stable

    if(displayConf != NULL) {
        drawLogo();
        busyWait(10000000);
        clearScreen();
    }

    return 0;
}

int deInitPicoVGA(bool closeDisplay, bool closeController, bool closeAudio, bool closeSD) {
    if(closeDisplay) {
        if(deInitDisplay()) return 1;
    }
    if(closeController) {
        if(deInitController()) return 1;
    }
    if(closeAudio) {
        if(deInitAudio()) return 1;
    }
    if(closeSD) {
        if(deInitSD()) return 1;
    }
    return 0;
}

static void drawLogo() {
    drawText(50, 250, 350, "pico-vga -- Michael Drobot, 2023", COLOR_WHITE, COLOR_BLACK, false, 0);
    drawText(50, 258, 350, "   Licensed under GNU GPL v3.", COLOR_WHITE, COLOR_BLACK, false, 0);
}