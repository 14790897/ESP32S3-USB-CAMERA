# ESP32-S3 USB UVC Camera with OV2640

This project implements a USB Video Class (UVC) camera using ESP32-S3 and OV2640 camera sensor with DVP interface.

## Hardware Requirements

- ESP32-S3 development board with USB OTG support
- OV2640 camera module with DVP interface
- Appropriate jumper wires for connections

## GPIO Connections

Connect the OV2640 camera to ESP32-S3 as follows:

| OV2640 Pin | ESP32-S3 GPIO | Function |
|------------|---------------|----------|
| SIOD (SDA) | GPIO 4        | I2C Data |
| SIOC (SCL) | GPIO 5        | I2C Clock |
| XCLK       | GPIO 15       | Master Clock |
| VSYNC      | GPIO 6        | Vertical Sync |
| HREF       | GPIO 7        | Horizontal Reference |
| PCLK       | GPIO 13       | Pixel Clock |
| D0 (Y2)    | GPIO 11       | Data bit 0 |
| D1 (Y3)    | GPIO 9        | Data bit 1 |
| D2 (Y4)    | GPIO 8        | Data bit 2 |
| D3 (Y5)    | GPIO 10       | Data bit 3 |
| D4 (Y6)    | GPIO 12       | Data bit 4 |
| D5 (Y7)    | GPIO 18       | Data bit 5 |
| D6 (Y8)    | GPIO 17       | Data bit 6 |
| D7 (Y9)    | GPIO 16       | Data bit 7 |
| VCC        | 3.3V          | Power |
| GND        | GND           | Ground |

**Note:** PWDN and RESET pins are not used in this configuration (set to -1).

## Features

- USB Video Class (UVC) compliant
- MJPEG video streaming
- VGA resolution (640x480)
- Plug-and-play with standard UVC drivers
- No additional drivers required on host computer

## Building and Flashing

1. Make sure you have ESP-IDF installed and configured
2. Set the target to ESP32-S3:
   ```bash
   idf.py set-target esp32s3
   ```
3. Build the project:
   ```bash
   idf.py build
   ```
4. Flash to the ESP32-S3:
   ```bash
   idf.py flash monitor
   ```

## Usage

1. Connect the OV2640 camera to ESP32-S3 according to the GPIO table above
2. Flash the firmware to ESP32-S3
3. Connect ESP32-S3 to computer via USB cable (use the USB port that supports OTG)
4. The device should appear as a standard USB camera
5. Open any camera application (e.g., Camera app on Windows, Photo Booth on macOS, guvcview on Linux)
6. Select the ESP32-S3 camera and start streaming

## Configuration

The camera settings can be modified in the `camera_config` structure in `main.c`:

- **Frame size**: Change `FRAMESIZE_VGA` to other supported sizes
- **JPEG quality**: Adjust `jpeg_quality` (1-63, lower = better quality)
- **Frame rate**: Modify `xclk_freq_hz` and UVC frame descriptors

## Troubleshooting

1. **Camera not detected**: Check GPIO connections and power supply
2. **No video stream**: Verify USB cable supports data transfer and ESP32-S3 USB OTG port is used
3. **Poor image quality**: Adjust JPEG quality setting or lighting conditions
4. **Build errors**: Ensure all required components are properly installed in ESP-IDF

## Technical Details

- **Video Format**: Motion JPEG (MJPEG)
- **Resolution**: 640x480 (VGA)
- **Frame Rate**: Up to 30 FPS (depending on lighting and USB bandwidth)
- **USB Interface**: USB 2.0 Full Speed
- **Memory**: Uses PSRAM for frame buffers

## Dependencies

- ESP-IDF (version 5.0 or later)
- esp32-camera component
- TinyUSB component (included in ESP-IDF)

## License

This project is licensed under the Apache License 2.0.
