#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/pio.h"

#include "build/vga.pio.h"

#define LED_PIN 1

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

/*
TODO:
    signal color to start from vsync code (since the
    first hsync comes after the first data)
    Currently color starts two cycles too late
*/

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

struct PIO_data{
    PIO pio;
    uint offset;
    uint sm;
};
typedef struct PIO_data PIO_data;

// Verify pin numbers by blinking them
void test_pins(){
    // Enable pins
    // Since dir=1 means output, we can repurpose the mask
    uint32_t mask =
        (1 << PHSYNC) |
        (1 << PVSYNC) |
        (1 << PRED)   |
        (1 << PGREEN) |
        (1 << PBLUE);
    
    gpio_init_mask(mask);
    gpio_set_dir_masked(mask, mask);

    while(1){
        printf("%d\n", mask);
        gpio_put_masked(mask, mask);
        sleep_ms(250);
        gpio_put_masked(mask, 0);
        sleep_ms(250);
    }
}

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

void init_pio(PIO_data* piod){
    piod->pio = pio0;

    // Find unused sm
    piod->sm = pio_claim_unused_sm(piod->pio, true);

    // Load program
    piod->offset = pio_add_program(piod->pio, &vga_program);
}

int main(){
    // Clock to 42.35 MHz
    set_sys_clock();
    // For USB serial
    stdio_init_all();

    for(int i=0; i<20; i++){
        printf("Waiting... (%d)\n", i);
        sleep_ms(500);
    }
    // PIO
    PIO_data piod;
    init_pio(&piod);
    // Start program
    vga_program_init(piod.pio, piod.offset);
    gpio_put(PVSYNC, 1);
    gpio_put(PHSYNC, 1);

    printf("offset: %d\n", piod.offset);

    uint8_t vsync_pc = 0;
    pio_sm_put_blocking(piod.pio, piod.sm, 0x0);
    while(1){
        
    }
}
