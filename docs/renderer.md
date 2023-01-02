# Renderer
How the renderer works, how the DMA/PIO subsystem works, and how you get VGA video on your screen.

## The VGA Spec
VGA is an analog video signal protocol. There are 15 pins on the connector, but only 5 matter: Red, Green, Blue, Horizontal Sync, and Vertical Sync.

The hardware description of VGA is in [hardware.md](./hardware.md), but the TL;DR is that the Pico only needs to control those 5 lines to make any VGA display work. The Horizonal and Vertical Sync timings need to match those in the official spec, found on [TinyVGA](http://tinyvga.com/vga-timing). Hardware is responsible for translating the Pico's digital color signals to analog, the Pico just needs to handle outputting [8 bit color](https://en.wikipedia.org/wiki/8-bit_color) through some GPIO pins.

## Making the VGA Signal -- The PIO System
The Pico's timer system is not very flexible -- it only has a microsecond-based timer and a cycle-accurate PWM system, but starting these in software causes many problems, the main one being that starting clock signals in sync is basically impossible.

The solution is to use the Programmable IO (PIO) system built in to the Pico. This allows the programmer to create custom IO systems that are cycle-accurate to the system clock and *extremely* fast. It also allows single-cycle delays for synchronization and perfectly simultaneous starting.

### The System Clock
Another important part of the PIO system is the system clock itself. 800x600 VGA at 60Hz runs at a pixel clock (the rate pixels are pushed/the color lines are sampled) of 40MHz. Making the system clock an exact multiple of 40MHz makes clock generation for Sync lines and color significantly easier. Use [vcocalc.py](../scripts/vcocalc.py) to calculate the constants required to set the system clock, and then call the function in the main program.

### HSync and VSync
The Horizontal and Vertical Sync PIO programs are essentially glorified clock generators, but they are what allow the display to detect the signal as 800x600, 640x480, etc. They count a very specific number of clock cycles, pull a GPIO line high, count a very specific number of cycles, then pull it low again. The timings were calculated from some math done on the timings on TinyVGA and dialed in and verified using an oscilloscope and much trial and error. Both sets of timings are multipliers of the pixel clock of the base resolution used (in the case of 800x600, it's 40MHz).

### Color
Color, at least from the PIO side, is very simple. The first thing to understand is the flow of data in the PIO state machine system:
```
                        (PULL)                     (OUT)
System/Memory --> TX FIFO --> Output Shift Register --> Pins
                 (4x32 bit)         (1x32 bit)

                        (PUSH)                     (IN)
System/Memory <-- RX FIFO <-- Input Shift Register <-- Pins
                 (4x32 bit)         (1x32 bit)
```
The color state machine is very simple: All it does is take data from the TX FIFO, pull it to the Output Shift Register automatically (see: Autopull in Pico docs) and, every clock cycle, OUT 8 bits of data from the Output Shift Register to 8 GPIO pins.

## DMA -- Feeding the Color State Machine
By far, the hardest of this system is feeding the Color state machine. It may seem simple: just tell DMA to constantly copy data from a buffer in memory to the TX FIFO of the Color state machine.

There are a few problems with this, but the biggest one is that there is blank time on the right and bottom of the screen during the front and back porches and sync pulses (see [hardware.md](./hardware.md)). This means any color data outputted during this time can, at best, not show up, be wasted, and mess up the next line. At worst, the display doesn't understand what the board is doing and make the full frame out of sync. There are a few ways to stop the color data from being sent:
- Put in placeholder data (zeros or garbage)
- Stop the PIO state machine by disabling it, either through the enable flag or an IRQ command in the PIO assembly code itself
- Stop the DMA by disabling it (stall the PIO state machine while it waits for data)


The second major problem is that the DMA doesn't understand that it needs to loop back to the beginning of the memory buffer when it reaches the end. It will keep going until it happens to stop or reaches an invalid memory address.

The third major problem is if this depends on a buffer in memory (representing the frame to be displayed), what happens when a single frame gets larger than the memory on the Pico? A single 640x480 frame, stored at 8 bit color (8 bits/1 byte per pixel) is just above 307kB. 800x600 is 480kB. The Pico has only 264kB of RAM, so some kind of compromise must be made. Also, if the Pico is not outputting full resolution (say, 400x300, 1/4 of 800x600), there need to be extra calculations for line and pixel doubling.

Finally, almost as big of an issue as the first, everything needs to happen **EXTREMELY** quickly. The CPU is inherently unreliable; the first iteration of the DMA system had DMA be reconfigured with an interrupt after each line during the blanking time after every line. However, the amount of time it took the CPU to acknowledge the interrupt, execute the code, and restart the DMA channel was *too slow* and the frame would get out of sync. Countless hours were spent making the DMA as CPU-independent as possible and the reconfiguration interrupt as fast and fail-proof as possible.

The amount of time spent on figuring out these issues... I don't even want to talk about it. The solution is what is currently in the SDK, explained below.

### The Arrays
There are **3** arrays that make up the DMA system: `frame`, representing the buffered frame, `BLANK`, representing a single blank line, and `frameReadAddr`, containing a pointer to the start of every line in memory.

#### `frame`
This contains the color data for the frame being displayed. It is in 8 bit color (1 byte per pixel) and the size of the frame displayed: if the resolution being displayed is 400x300, or 200x150, that will be the size of `frame` (it will not be 800x600, the base resolution).

#### `BLANK`
An array, the width of the frame (400 if using 400x300, or 200 if using 200x150), that is entirely set to zeros. Represents a blank line, and used as a placeholder for the `frame` data during the blanking time at the bottom of the screen. All 8 bit values.

#### `frameReadAddr`
The most complicated array. Contains pointers to every line of the frame being displayed, used as the read addresses for the DMA. Its size is the **TRUE**, base resolution, **height** of the display, **INCLUDING** the blank lines at the bottom -- regardless of if you are using 400x300, 200x150, or full 800x600 render resolution, this array will be 628 items long (600 lines + 28 blank lines).

The reason for this array is to handle line doubling. When running at 400x300, the processor needs to double each line and each pixel (display it twice) so the frame is the right size on the screen. Pixel doubling is easy -- slow down the clock of the color state machine, and each pixel will stay on the GPIO pins for longer. Line doubling is harder -- it means that each line needs to be fed to the color state machine twice. Since the DMA reads from this array to get its read addresses, doing line doubling is as simple as writing this array accordingly with the number of pointers required per line.

This also allows for line interpolation in the future for higher resolution rendering (like 800x600). A majority of the frame could be buffered in memory and then certain lines could be calculated on the fly and interpolated in between the buffered ones.

Example for a 400x300 frame (one line doubling):
```
frameReadAddr = {
1:   frame[0],
2:   frame[0],
3:   frame[1],
4:   frame[1],
5:   frame[2],
     ....
596: frame[298],
597: frame[298],
598: frame[299],
599: frame[299],
600: BLANK,
    ....
625: BLANK,
626: BLANK,
627: BLANK
};
```

### The DMA Channels
This was, by far, the hardest part of this project. The Color signal is extremely susceptible to getting out of sync, and once it does, it's impossible to get it back in sync without restarting DMA and PIO. It gets out of sync when a particular step of the process takes longer than it should, and this was a particular problem with any interrupt request to the CPU.

An interrupt sent to the CPU is less of a "do this now" and more of a "do this soon", and that difference is enough to mess up the DMA's tight timings. The time between when the interrupt was fired and when the interrupt request callback was run, the execution time of any code in the callback, and reconfiguring the DMA through Pico SDK functions was too much for the timings, and frame would always come out with a lot of artifacts.

After much trial and error, the solution was this:
```
DMA Ch 0:
- Read a 32-bit pointer from frameReadAddr
- Write it to the read address of Ch 1 and tell Ch 1 to start
- Increment the read address by 32 bits
- Call the IRQ

DMA Ch 1:
- Wait to be triggered
- Starting at the read address, transfer FRAME_WIDTH (400 for 400x300, 200 for 200x150) bytes to the color state machine's TX FIFO (32 bits/4 bytes at a time, regulated by requests from PIO)
- Once done, trigger DMA Ch 2

DMA Ch 2:
- Wait to be triggered
- Transfer zeros to the color state machine to fill the blanking time at the right of the frame
- Once done, trigger Ch 0

IRQ:
- Acknowledge interrupt
- If DMA Ch 0's read address (the frameReadAddr array) is greater than the end of the array,
  reset it to the beginning of frameReadAddr
```

What this does is make *almost* everything CPU-independent (run from the DMA instead of rely on the CPU to reconfigure stuff). This gives significantly better performance, much better reliability, and a functioning graphics card. The IRQ is also extremely fast and it's based on the actual read address of the DMA channel rather than some counter variable (It tended to break when I was counting lines with a line counter variable).

## The 2D Renderer
The renderer (comparatively, at least) is very simple. Since the entire frame is buffered in system memory, writing to and modifying the frame is easy -- it's just a 2D array. The renderer is based on the Pico having a second core. The second core, Core 1, is entirely dedicated to running the renderer and handling DMA reconfiguration.

### The Render Queue
The render queue is a linked list of RenderQueueItem elements. Each element has parameters, coordinates, a color, and an identifier. In addition to holding the data for each element, the order in which the list is linked is the order in which things are rendered. This means elements can be "under" or "over" others.

### Rasterization
The render function uses standard rasterization functions to draw lines and circles. Since the render queue represents vectors and not pixels, some math needs to be done to convert the two endpoints of a line, for example, into pixels on the frame. That is the rasterization process.

### Render Modes: Manual vs. AutoRender
The renderer has two different modes: Manual mode and AutoRender mode. Manual mode is pretty much what you think it is: The programmer makes changes to the render queue or render queue items and then calls an update function which activates the renderer and completely redraws the frame. This is used if there are a lot of changes being made to the render queue and you don't want to hog the second core, if you are doing other things on the second core and you want to save some resources, or you want more control over when things are displayed.

The AutoRender mode constantly loops over the render queue looking for items that need to be updated. When it finds one, it redraws that item and anything after it. This is because when an item is redrawn, it is drawn on the "top" layer of the frame, which might not be where the programmer wants it. This requires more CPU time on the second core, but is more convenient for the programmer.

## The 3D Renderer
(Coming soon!)

## The Limits of the Raspberry Pi Pico
The capabilities of Pico-VGA are limited mainly by three things: The system clock speed, memory usage, and processor execution speed.

### System Clock Speed
The Pico executes at 133MHz by default. This means that each instruction is executed on one of the edges of the clock signal (let's say rising edge for now). This means that the Color state machine can only run an OUT command (output data to the pins) at 133/2MHz, or 66.5MHz. This caps Pico-VGA's resolution at a pixel clock of 66.5 MHz. With overclocking, it could potentially go faster and push higher resolutions, but that's outside the scope of Pico-VGA.

### Memory Usage
This has already been discussed in the beginning of the DMA section, but a single frame of 800x600 video at 8 bit color depth is 480kB, nearly double the Pico's memory capacity. This means that a frame buffer is not possible, at least for a full frame. Well, this could be worked around by calculating individual lines on the fly, outputting them, and then reusing those individual line arrays for the next set of lines, right?

### Processor Execution Speed (IPS/Instructions Per Second)
Calculating lines on the fly leads to another issue that only shows itself when you're working on the scale of CPU cycles: To calculate lines on the fly, the next line must be calculated and stored while the previous is being displayed. This means that the next line must be fully calculated in, at *most*, the display time of the previous line. When you break a for loop, for example, into assembly and look at the number of cycles it takes relative to the number of cycles the CPU has to calculate the line, it makes it basically impossible even with both cores working. The CPU simply can't keep up.

## The Processing Time/Memory Usage Balance
The balance between memory usage and processor usage is extremely important here. The less memory you want to use, the more lines you have to calculate on the fly and the more processor cycles you have to use. If you want to use less processing power, you have to store more lines (or even an entire frame) in memory. That way nothing (or very little) has to be recalculated between line or frame refreshes on the display.

Which way the balance leans is dependent on how much memory the programmer needs, how much CPU power the programmer needs, what resolution is being used, and the limitations of the RP2040/Raspberry Pi Pico itself.

This is where line interpolation comes in. This is a system that will be implemented in the future, but it takes in a memory size cap and a base resolution. It saves as much of the frame in memory as the memory cap allows and interpolates the rest of the lines with on-the-fly-calculated color data. This is the only way to get any more than a static frame at resolutions higher than what the Pico's memory capacity allows.