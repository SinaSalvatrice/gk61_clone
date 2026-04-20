# gk61_clone Notes

- Matrix with WS2812 RGB Matrix on GP29 (36 LEDs), no encoder or OLED.
- RP2040 Zero microcontroller.
- Matrix rows: 0, 1, 2, 3, 4
- Matrix columns: 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 26, 27, 28
- Matrix is fully routed on the RP2040, no expander required.
- RGB Matrix uses an approximate 36-zone map rather than one LED per switch.
- Layout matches GK61 (see reference image for mapping).
- RGB Matrix is enabled in rules.mk with keypress-reactive effects.
- Build output: UF2 file for RP2040 bootloader.

## dead esc
- ESC key sends no output
