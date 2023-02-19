#include "sdk.h"

/*
Ideas:
- GIMP can output C header, raw binary, etc files, make that the sprite format
*/

DisplayConfig_t * displayConfig;
ControllerConfig_t * controllerConfig;
AudioConfig_t * audioConfig;
SDConfig_t * sdConfig;
USBHostConfig_t * usbConfig;

static void drawLogo();

int initPicoVGA(DisplayConfig_t * displayConf, ControllerConfig_t * controllerConf, AudioConfig_t * audioConf,
                SDConfig_t * sdConf, USBHostConfig_t * usbConf) {
    if(displayConf != null) {
        displayConfig = displayConf;
        initDisplay();
    }
    if(controllerConf != null) {
        controllerConfig = controllerConf;
        initController(); //controller.c
    }
    if(audioConf != null) {
        audioConfig = audioConf;
        initAudio(); //audio.c
    }
    if(sdConf != null) {
        sdConfig = sdConf;
        initSD(); //sd.c
    }
    if(usbConf != null) {
        usbConfig = usbConf;
        initUSB(); //usb.c
    }
    
    busyWait(10000000); //wait to make sure everything is stable

    if(displayConf != null) {
        drawLogo();
        busyWait(10000000);
        clearScreen();
    }
}

static void drawLogo() {
    drawText(NULL, 50, 250, 350, "pico-vga -- Michael Drobot, 2023", COLOR_WHITE, COLOR_BLACK, false, 0);
    drawText(NULL, 50, 258, 350, "   Licensed under GNU GPL v3.", COLOR_WHITE, COLOR_BLACK, false, 0);
}