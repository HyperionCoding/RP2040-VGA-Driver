#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/pio.h"

#include "build/vga_rgb.pio.h"
#include "build/vga_hsync.pio.h"
#include "build/vga_vsync.pio.h"

/*
Timings: http://tinyvga.com/vga-timing/640x480@60Hz
Resolution: 640x480 @ 59.94Hz (god damn ntsc)
Whole resolution: 800x525
Pixel frequency: 25.175 MHz
Line frequency: 31.46875 kHz
Hsync: 800 pixels (one line)
Vsync: 525 lines

Vertical timing (lines):
Front porch: 10
Sync pulse: 2
Back porch: 33

Horizontal timing (pixels):
Front porch: 16
Sync pulse: 96
Back porch: 48
*/

/*
VERIFY LIST:
    clock speed             : correct
    color data frequency    : correct
    vsync frequency         : correct
    hsync frequency         : correct
*/

/*
Timing measurements (target):
    vsync freq:     59.98 Hz    (59.94 Hz) FINE
    vsync pulse:    63.50 us    (63.55 us) FINE
    vsync front:    319.5 us    (317.7 us)
        removing hsync back porch from measurement:
                    317.59 us FINE
    vsync back:     1.041 ms     (1.048 ms) !!!
        removing one hsync
                    1.071 ms
        adding hsync data back
                    1.045 ms PROBABLY FINE
    vsync off:      1.4247 ms   (1.43 ms) FINE

    hsync freq:     31.502 kHz  (31.469 kHz) FINE
    hsync pulse:    3.808 us    (3.813 us) FINE
    hsync count:    480         (480) FINE
*/

void set_sys_clock(){
    /* Calculated using 
    /pico-sdk/src/rp2_common/hardware_clocks/scripts/vcocalc.py
    Target: 2*25.175 MHz = 50.35 MHz
    Achieved: 50.40 MHz
    Error: 0.1 %
    */
    uint32_t vco_freq = 1512000000;
    uint post_div1 = 6;
    uint post_div2 = 5;
    set_sys_clock_pll(vco_freq, post_div1, post_div2);
}

void init_pio(PIO pio){
    // Add programs
    uint offset_rgb = pio_add_program(pio, &vga_rgb_program);
    uint offset_hsync = pio_add_program(pio, &vga_hsync_program);
    uint offset_vsync = pio_add_program(pio, &vga_vsync_program);

    // Init programs
    vga_rgb_program_init(pio, offset_rgb);
    vga_hsync_program_init(pio, offset_hsync);
    vga_vsync_program_init(pio, offset_vsync);
}

int main(){
    // Clock to 50.35 MHz
    set_sys_clock();
    // For USB serial
    stdio_init_all();

    // Waiting for usb-uart to connect
    printf("Waiting... \n");
    sleep_ms(5000);

    // Config PIO
    PIO pio = pio0;
    init_pio(pio);

    // Start pio
    pio_enable_sm_mask_in_sync(pio, 0b0111);

    while (1) {
    }
}
