# RP2040-VGA-Driver
Raspberry Pi Pico VGA Driver that utilises the PIO. The resolution is hardcoded to 640x480.

## Building
Run `cmake ..` and `make` in /build folder. The output including .uf2 is generated in the same folder.

## TODO
- Currently digital monitor complains about unsupported display mode
- Color data transmission starts at the wrong time
- Add DMA and autopull for color data
