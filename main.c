#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"

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

/*
1 bit color frame buffer for output. Each byte encodes
two pixels and thus it takes only very modest 153 kB.
More dense data would be possible but writing would be harder.
Format: rgb0rgb0
*/
#define FRAMEBUFFER_W 640/2
#define FRAMEBUFFER_H 480
uint8_t frame_buffer[FRAMEBUFFER_W][FRAMEBUFFER_H];

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

uint init_dma(void* source, uint32_t count){
    // RGB data DMA
    int channel = dma_claim_unused_channel(true);
    channel = 0;
    // Config
    dma_channel_config c = dma_channel_get_default_config(channel);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);
    channel_config_set_dreq(&c, DREQ_PIO0_TX0);

    volatile void* dest = &pio0_hw->txf[0];
    dma_channel_configure(
        channel,    // Channel to be configured
        &c,         // The configuration we just created
        dest,       // The initial write address
        source,     // The initial read address
        count,      // Number of transfers; in this case each is 1 byte.
        false       // Start immediately.
    );
    return channel;
}

void __isr DMA_irq_handler(){
    irq_clear(DMA_IRQ_0);
    dma_hw->ints0 = 1 << 0;
    dma_channel_set_read_addr(0, &frame_buffer, true);
}


void fill_frame_buffer(){
    for(int x=0; x<FRAMEBUFFER_W; ++x){
        for(int y=0; y<FRAMEBUFFER_H; ++y){
            if (x < 80) {
                frame_buffer[x][y] = 0x0F;
            } else if (x < 160) {
                frame_buffer[x][y] = 0xFF;
            }
        }
    }
}

int main(){
    // Clock to 50.35 MHz
    set_sys_clock();
    // For USB serial
    stdio_init_all();
    
    fill_frame_buffer();

    // Waiting for usb-uart to connect
    sleep_ms(5000);

    // Configure PIO
    PIO pio = pio0;
    init_pio(pio);

    // Configure DMA, 32 bits transfer size thus /4
    uint dma_channel = init_dma(&frame_buffer, sizeof(frame_buffer)/4);
    
    // Start pios
    pio_enable_sm_mask_in_sync(pio, 0b0111);

    dma_channel_set_irq0_enabled(dma_channel, true);
    irq_set_exclusive_handler(DMA_IRQ_0, DMA_irq_handler);
    irq_set_enabled(DMA_IRQ_0, true);

    // Call once to start DMA
    DMA_irq_handler();

    printf("Start\n");
    while (1) {
    }
}