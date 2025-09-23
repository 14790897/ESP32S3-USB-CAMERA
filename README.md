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


C:\Program Files (x86)\NVIDIA Corporation\PhysX\Common;C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.6\bin;C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.6\libnvvp;C:\Program Files (x86)\VMware\VMware Workstation\bin\;C:\git-program\apache-maven-3.8.8-bin\apache-maven-3.8.8\bin;C:\gradle\gradle-7.6.2-all\gradle-7.6.2\bin;C:\spring-boot-cli\spring-boot-cli-3.1.4-bin\spring-3.1.4\bin;C:\Program Files\Java\jdk-11;C:\WINDOWS\system32;C:\WINDOWS;C:\WINDOWS\System32\Wbem;C:\WINDOWS\System32\WindowsPowerShell\v1.0\;C:\WINDOWS\System32\OpenSSH\;C:\Program Files\dotnet\;C:\Program Files\Git\cmd;C:\Program Files\MySQL\MySQL Server 8.1\bin;C:\ProgramData\chocolatey\bin;C:\Users\13963\AppData\Local\Android\Sdk\platform-tools;C:\Program Files (x86)\Windows Kits\10\Windows Performance Toolkit\;C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin;C:\Program Files\Java\jdk-11\bin;C:\git-program\apache-maven-3.8.8-bin\apache-maven-3.8.8;C:\Program Files\Go\bin;C:\Program Files\Redis\;C:\Program Files\Graphviz\bin;C:\Users\13963\AppData\Roaming\nvm;C:\Program Files\nodejs;C:\Program Files\CMake\bin;C:\Program Files (x86)\ZeroTier\One\;C:\Strawberry\c\bin;C:\Strawberry\perl\site\bin;C:\Strawberry\perl\bin;C:\Program Files\NVIDIA Corporation\NVIDIA app\NvDLISR;C:\Program Files\NVIDIA Corporation\Nsight Compute 2024.3.2\;C:\Program Files (x86)\cloudflared\;C:\Program Files\Intel\WiFi\bin\;C:\Program Files\Common Files\Intel\WirelessCommon\;%SystemRoot%\System32\Wbem;C:\git-program\esp-idf\tools;C:\git-program\esp-idf\components\esptool_py\esptool;C:\Program Files\PuTTY\;%USERPROFILE%\AppData\Local\Muse Hub\lib;C:\Program Files\PostgreSQL\17\bin;C:\git-program\cockroach-sql-v25.2.5.windows-6.2-amd64;C:\Program Files (x86)\Microsoft SQL Server\160\Tools\Binn\;C:\Program Files\Microsoft SQL Server\160\Tools\Binn\;C:\Program Files\Mic


