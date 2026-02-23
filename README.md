# ESP32-S3 + WeAct 2.13" E-Paper (ESP-IDF)

Test project for **WeAct 2.13" black/white e-paper** with **ESP32-S3** using ESP-IDF.

The project includes:
- SPI initialization for the display
- Simple framebuffer
- Simple text rendering (5x7 font) with scaling
- Full refresh of the e-paper panel
- Wiring diagram in `docs/epaper_wiring.svg`

## Hardware

- ESP32-S3
- WeAct 2.13" E-Paper (BW)
- 3.3V logic

## Pinout (current)

| ESP32-S3 | E-Paper |
|---|---|
| GPIO6 | DIN / SDA (MOSI) |
| GPIO4 | CLK (SCK) |
| GPIO5 | CS |
| GPIO1 | DC |
| GPIO2 | RST |
| GPIO3 | BUSY |
| 3V3 | VCC |
| GND | GND |

`MISO` is not used.

## Build and flash

Run in the project folder:

```bash
source $IDF_PATH/export.sh
idf.py set-target esp32s3
idf.py build
idf.py flash monitor
```

## Customization

### Text size

Change in `main/main.c`:

- `TEXT_SCALE` (for example `2`, `3`, `4`)

### Text content

Change the strings in `app_main()` in `main/main.c`.

### SPI speed

Change `EPD_SPI_HZ` in `main/main.c`.

## Troubleshooting

### Only half of the screen updates / dot pattern

The code uses the panel's internal RAM format (`122x250`) and rotates coordinates in software. If the issue comes back, verify that these dimensions were not changed in `main/main.c`.

### No update at all

- Verify BUSY/CS/DC/RST wiring
- Confirm 3.3V supply
- Try lower SPI speed (for example `2000000`)

### Image is upside down / wrong rotation

Rotation is handled in `epd_draw_pixel()` in `main/main.c`. You can adjust the mapping there for other panel revisions.

## Files

- `main/main.c` - driver + demo
- `docs/epaper_wiring.svg` - wiring diagram
- `.gitignore` - ESP-IDF/IDE ignore

