# ESP32-S3 + WeAct 2.13" E-Paper (ESP-IDF)

Test project for **WeAct 2.13" black/white e-paper** with **ESP32-S3** using ESP-IDF.

The project includes:
- A reusable e-paper driver component (`components/epaper_driver`)
- SPI initialization for the display
- Simple framebuffer
- Simple text rendering (5x7 font) with scaling
- Full refresh of the e-paper panel
- Partial refresh API for region updates
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

### SPI speed and pins

Change driver config in `main/main.c`:

```c
epaper_driver_config_t cfg = epaper_driver_default_config();
cfg.spi_hz = 4000000; // lower if needed
// cfg.pin_mosi, cfg.pin_clk, cfg.pin_cs, cfg.pin_dc, cfg.pin_rst, cfg.pin_busy
ESP_ERROR_CHECK(epaper_driver_init(&epaper, &cfg));
```

## Reuse in other projects

Copy this component folder into another ESP-IDF project:

- `components/epaper_driver/`

Then include and use:

```c
#include "epaper_driver.h"

epaper_driver_t epaper;
epaper_driver_config_t cfg = epaper_driver_default_config();
ESP_ERROR_CHECK(epaper_driver_init(&epaper, &cfg));

epaper_driver_clear(&epaper, true);
epaper_driver_draw_text_scaled(&epaper, 8, 10, "Hello", 3);
ESP_ERROR_CHECK(epaper_driver_update_full(&epaper));
```

And make sure your app component has:

```cmake
idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES epaper_driver)
```

### Partial refresh (region-based)

You can create a partial-refresh object for a region and update only that area:

```c
epaper_partial_refresh_t part;
ESP_ERROR_CHECK(epaper_partial_refresh_init(&part, &epaper, 8, 10, 120, 40));

// Draw into the same framebuffer as usual
epaper_driver_draw_text_in_rect(
    &epaper,
    part.area.x, part.area.y, part.area.width, part.area.height,
    "Updated",
    2,
    true   // clear to white before drawing
);

// Refresh only the configured region
ESP_ERROR_CHECK(epaper_partial_refresh_update(&part));
```

Note: partial refresh now uses separate previous/current framebuffers internally. This avoids text corruption where partial updates could overwrite already visible content.

Available types/functions:

- `epaper_rect_t`
- `epaper_partial_refresh_t`
- `epaper_driver_draw_text_in_rect(...)`
- `epaper_partial_refresh_init(...)`
- `epaper_partial_refresh_update(...)`

## Troubleshooting

### Only half of the screen updates / dot pattern

The driver uses the panel's internal RAM format (`122x250`) and rotates coordinates in software. If the issue comes back, verify that these dimensions were not changed in `components/epaper_driver/epaper_driver.c`.

### No update at all

- Verify BUSY/CS/DC/RST wiring
- Confirm 3.3V supply
- Try lower SPI speed (for example `2000000`)

### Image is upside down / wrong rotation

Rotation is handled in `epaper_driver_draw_pixel()` in `components/epaper_driver/epaper_driver.c`. You can adjust the mapping there for other panel revisions.

### Partial refresh does not look correct

- Start with a full refresh (`epaper_driver_update_full`) after init
- Use `epaper_driver_draw_text_in_rect(..., clear_white=true)` for text replacement in a region
- Ensure the partial region covers all changed pixels
- If your panel revision behaves differently, tune update control in `epaper_partial_refresh_update()` in `components/epaper_driver/epaper_driver.c`

## Files

- `components/epaper_driver/include/epaper_driver.h` - reusable public driver API
- `components/epaper_driver/epaper_driver.c` - reusable driver implementation
- `components/epaper_driver/CMakeLists.txt` - component registration
- `main/main.c` - demo app using the driver
- `docs/epaper_wiring.svg` - wiring diagram
- `.gitignore` - ESP-IDF/IDE ignore
