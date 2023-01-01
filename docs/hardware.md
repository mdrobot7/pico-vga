# Hardware
How the hardware makes the software work, explanations for particular parts choices made, and why the boards turned out how they did. Also resources on learning about the VGA protocol.

## VGA
The best resource on how the VGA protocol works is Ben Eater's videos on YouTube making a graphics card in hardware. Here are [Part One](https://www.youtube.com/watch?v=l7rce6IQDWs), building the HSync and VSync logic, and [Part Two](https://www.youtube.com/watch?v=uqY3FMuMuRo), building the color DAC and testing. The diagram of the sync pulses is [here](https://global.discourse-cdn.com/digikey/original/2X/4/43a91de5f1cc2ab380b22c4758b8b408da97e0c2.jpeg). The timings can be found on [TinyVGA](http://tinyvga.com/vga-timing).

These videos and diagrams are the best resources on the VGA signal I have found, and I used all of them extensively throughout this project. He explains the logic and protocol very well, including how line and pixel doubling work. All of his videos are highly recommended if you are into hardware, and hardware logic.

## The Pico-VGA Implementation
Out of the 15 pins in the VGA DSUB connector, only 5 matter: Red, Green, Blue, Horizontal Sync, and Vertical Sync.

### HSync and VSync
These are simple digital GPIO TTL lines. The timings are complex, but for more information see [renderer.md](./renderer.md).

### Color
VGA is an analog signal, which means the color values are a voltage spectrum rather than a digital 1 or 0. The Pico only knows digital outputs, which means these 1s and 0s (~3.3v and ~0v) need to be converted to a range of voltages. Per VGA spec, this range needs to be between 0v and 0.7v, where 0v means "off" and 0.7v means "full brightness" for each color of each pixel.

The good news is that you can divide this voltage range up, do some math, and get resistor values to make a resistor-based DAC. This is because there is a 75 Ohm resistor inside the display which makes a resistor divider. Solving the resistor divider formula for R1, with R2, the source voltage (3.3v) and the output voltage (0-0.7v) known gives that resistor value. They are listed below:
```
Red/Green, 3 bits/8 states of color data:
    0 0 0 = 0v
    0 0 1 = 0.1v = 2400R
    0 1 0 = 0.2v = 1162.5R
    0 1 1 = 0.3v = 750R
    1 0 0 = 0.4v = 543.75R
    1 0 1 = 0.5v = 420R
    1 1 0 = 0.6v = 337.5R
    1 1 1 = 0.7v = 278.57R

  With real resistor values, E20 series (voltage is the actual outputted voltage):
    0 0 0 = ------ = 0v
    0 0 1 = 2200R  = 0.10879v
    0 1 0 = 1200R  = 0.19411v
    0 1 1 = 776.4R = 0.32781v -- parallel of 2200R and 1200R
    1 0 0 = 503R   = 0.42820v -- 470R and 33R in series
    1 0 1 = 409.4R = 0.51094v -- parallel of 503R and 2200R
    1 1 0 = 354.4R = 0.57639v -- parallel of 503R and 1200R
    1 1 1 = 305.3R = 0.65080v -- parallel of all 3

    - Note: These numbers don't exactly line up for some reason (001 + 010 in parallel doesn't add to 011's resistance.) Don't know why, but in the videos Ben Eater just picks the closest actual resistor and it ends up closer than the math. So that's what we're doing here.

Blue, 2 bits/4 states:
    0 0 = 0v
    0 1 = 0.23v = 1001.08R
    1 0 = 0.47v = 451.6R
    1 1 = 0.7v  = 278.57R
  
  With real resistor values (voltage is actual outputted voltage):
    0 0 = ------ = 0v
    0 1 = 1000R  = 0.23023v
    1 0 = 437R   = 0.48339v -- 390R and 47R in series
    1 1 = 304.1R = 0.65286v -- parallel of 1000R and 437R (really close to the R and G end voltages)

 - Red channel
    - 2.2k
    - 1.2k
    - 470R
    - 33R
 - Green channel
    - 2.2k
    - 1.2k
    - 470R
    - 33R
 - Blue channel
    - 1k
    - 390R
    - 47R
```