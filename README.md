# AirSpot Firmware

AirSpot is a CO2 monitoring device firmware built for the nRF52832 microcontroller. This firmware enables real-time CO2 monitoring, data logging, BLE connectivity, and factory testing capabilities.

## 📋 Table of Contents

- [Hardware Requirements](#hardware-requirements)
- [Software Requirements](#software-requirements)
- [Project Structure](#project-structure)
- [Getting Started](#getting-started)
- [Building the Project](#building-the-project)
- [Flashing the Firmware](#flashing-the-firmware)
- [Features](#features)
- [Configuration](#configuration)
- [Factory Testing](#factory-testing)
- [DFU (Device Firmware Update)](#dfu-device-firmware-update)
- [Troubleshooting](#troubleshooting)
- [Contributing](#contributing)
- [License](#license)

## 🔧 Hardware Requirements

- **MCU**: Nordic nRF52832 (custom board)
- **CO2 Sensor**: Sensirion SCD4x series
- **Display**: ST7301 LCD controller
- **Flash Memory**: ZB25D16 SPI flash
- **Debug/Flash Tool**: J-Link debugger or compatible (Segger J-Link, J-Link EDU, etc.)

### Pin Configuration

The pin configuration is defined in `custom_board.h`. Key peripherals:

- **I2C**: CO2 sensor communication
- **SPI**: Flash memory and LCD
- **BLE**: Bluetooth Low Energy communication
- **ADC**: Battery voltage monitoring

## 💻 Software Requirements

### Required Tools

1. **Keil MDK-ARM (µVision)** v5.x or later

   - Download from: [ARM Keil Downloads](https://www.keil.com/download/)
   - License: Free evaluation license available for limited code size

2. **Nordic nRF5 SDK v17.1.0**

   - Already included in this repository at `nRF5_SDK_17.1.0_ddde560/`
   - Official source: [Nordic Semiconductor](https://www.nordicsemi.com/Products/Development-software/nrf5-sdk)

3. **J-Link Software and Documentation Pack**

   - Download from: [Segger J-Link](https://www.segger.com/downloads/jlink/)
   - Required for programming and debugging

4. **nRF Command Line Tools**
   - Download from: [Nordic nRF Tools](https://www.nordicsemi.com/Products/Development-tools/nrf-command-line-tools)
   - Required for DFU package generation and advanced operations

### Optional Tools

- **nRF Connect for Desktop** - For BLE debugging and testing
- **SEGGER RTT Viewer** - For real-time logging (already included in J-Link tools)

## 📁 Project Structure

```
airspot/
├── nRF5_SDK_17.1.0_ddde560/           # Nordic SDK
│   └── examples/
│       └── ble_peripheral/
│           └── ble_app_uart_minico2/  # Main project directory
│               ├── main.c              # Main application entry
│               ├── battery.c/h         # Battery management
│               ├── button.c/h          # Button handling
│               ├── protocol.c/h        # BLE protocol implementation
│               ├── user.c/h            # User settings and state
│               ├── factory_test_task.c # Factory testing module
│               ├── scd4x/              # CO2 sensor driver
│               │   ├── co2.c/h         # CO2 measurement logic
│               │   └── driver_scd4x.*  # SCD4x sensor driver
│               ├── lcd/                # LCD and UI components
│               │   ├── st7301.c/h      # LCD controller driver
│               │   ├── ui.c/h          # UI framework
│               │   └── ui_task.c       # UI task management
│               ├── history/            # Data logging to flash
│               │   ├── history.c/h     # Historical data management
│               │   ├── cfg_fstorage.c/h# Configuration storage
│               │   ├── flash_spi.c/h   # SPI flash driver
│               │   └── zb25d16.c/h     # Flash chip driver
│               └── pca10040/s112/      # Build configurations
│                   └── arm5_no_packs/  # Keil project files
│                       └── ble_app_uart_pca10040_s112.uvprojx
├── DFU_PKG/                           # Device Firmware Update packages
│   ├── public_key.c                   # DFU verification public key
│   ├── generate.sh                    # DFU package generation script
│   └── *.hex                          # Firmware hex files
└── README.md                          # This file
```

## 🚀 Getting Started

### 1. Clone the Repository

```bash
git clone <your-repo-url>
cd airspot
```

### 2. Install Required Software

Install all software listed in [Software Requirements](#software-requirements) above.

### 3. Open the Project

1. Launch Keil µVision
2. Open the project file:
   ```
   nRF5_SDK_17.1.0_ddde560/examples/ble_peripheral/ble_app_uart_minico2/pca10040/s112/arm5_no_packs/ble_app_uart_pca10040_s112.uvprojx
   ```

### 4. Configure Your Environment

Ensure the following paths are correctly set in Keil µVision:

- **Project → Options for Target → C/C++ → Include Paths**: SDK paths should be relative
- **Project → Options for Target → Debug**: J-Link selected as debugger
- **Project → Options for Target → Utilities**: Programming algorithm configured for nRF52832

## 🔨 Building the Project

### Using Keil µVision IDE

1. Open the project in Keil µVision
2. Select the build target: **nrf52832_xxaa**
3. Click **Project → Build Target** or press `F7`
4. The output hex file will be generated in:
   ```
   pca10040/s112/arm5_no_packs/_build/nrf52832_xxaa.hex
   ```

### Build Output

After a successful build, you'll find:

- `nrf52832_xxaa.hex` - Main firmware
- `nrf52832_xxaa.axf` - Debug symbols
- `nrf52832_xxaa.map` - Memory map

## 📲 Flashing the Firmware

### Method 1: Using Keil µVision (Recommended for Development)

1. Connect J-Link debugger to your device
2. In Keil: **Flash → Download** or press `F8`
3. The firmware will be programmed automatically

### Method 2: Using nRFgo Studio / nRF Connect Programmer

1. Connect J-Link debugger
2. Open nRF Connect Programmer
3. Select your device
4. Load the hex file: `_build/nrf52832_xxaa.hex`
5. Click "Write"

### Method 3: Using J-Link Commander (Command Line)

```bash
JLinkExe -device nRF52832_xxAA -if SWD -speed 4000
J-Link> connect
J-Link> loadfile nrf52832_xxaa.hex
J-Link> r
J-Link> g
J-Link> exit
```

### Method 4: Complete Factory Flash (with Bootloader & SoftDevice)

For production devices, use the complete package:

```bash
cd DFU_PKG
# This will flash SoftDevice + Bootloader + Application
nrfjprog --program final.hex --chiperase --verify --reset
```

## ✨ Features

### Core Features

- **Real-time CO2 Monitoring**: Continuous CO2, temperature, and humidity measurement
- **Power Management**: Multiple power modes (On-demand, Low, Mid, High)
- **Data Logging**: Historical data storage to external SPI flash
- **BLE Connectivity**: Full BLE UART service for mobile app communication
- **LCD Display**: Real-time display with graphs and status
- **Battery Monitoring**: Real-time battery voltage and charging detection
- **Automatic Self-Calibration (ASC)**: Baseline CO2 calibration
- **Alarm System**: Configurable CO2 level alarms with buzzer/vibrator
- **Do Not Disturb Mode**: Scheduled quiet periods
- **Factory Testing**: Comprehensive hardware validation system

### BLE Protocol

The device uses a custom protocol over BLE UART service for:

- CO2 data retrieval
- Configuration management
- Historical data download
- Factory testing commands
- DFU (Device Firmware Update)

Protocol details are in `protocol.c/h`.

## ⚙️ Configuration

### Device Configuration

Configuration is stored in flash memory and persists across reboots. Main settings:

```c
// Defined in cfg_fstorage.h
- Device name
- Power mode
- Alarm thresholds
- Vibrator/Buzzer enable
- Auto-calibration settings
- Do Not Disturb schedule
- Graph display ranges
- Bluetooth enable/disable
```

### Compile-Time Configuration

Key configurations in `sdk_config.h`:

- BLE parameters (advertising interval, connection parameters)
- UART settings for debug logging
- Timer configurations
- Memory allocation

### Application Configuration

Edit `custom_board.h` for:

- GPIO pin assignments
- Hardware feature enables/disables
- Peripheral configurations

## 🏭 Factory Testing

The firmware includes a comprehensive factory test system documented in `FACTORY_TEST_README.md`.

### Test Categories

1. **Automatic Electronic Tests**:

   - Sensor communication test
   - CO2 reading validation
   - Flash memory read/write test
   - Battery voltage test
   - LF crystal oscillator test
   - LCD controller test

2. **Manual Tests** (via mobile app):
   - Charge detection
   - Screen edge test
   - Button functionality
   - Buzzer test
   - Vibrator test
   - Case visual inspection

### Running Factory Tests

Factory tests are initiated via BLE commands from the mobile app. See `FACTORY_TEST_README.md` for detailed protocol information.

## 🔄 DFU (Device Firmware Update)

### Generating DFU Packages

The device supports secure DFU updates. To generate a DFU package:

```bash
cd DFU_PKG
./generate.sh
```

This will create:

- DFU .zip package for OTA updates
- Settings page for bootloader

### DFU Public Key

The DFU public key is stored in `DFU_PKG/public_key.c` and is compiled into the bootloader. This key verifies the signature of firmware updates.

**Important**: The corresponding private key must be kept secure and is NOT included in this repository.

### Performing DFU Updates

1. **Via nRF Connect Mobile App**:

   - Open nRF Connect
   - Connect to device
   - Select DFU
   - Choose the .zip package
   - Start update

2. **Via Custom Mobile App**:
   - Use the Nordic DFU library
   - Implement as per `ble_dfu.c` protocol

## 🐛 Troubleshooting

### Build Issues

**Problem**: Include paths not found

- **Solution**: Check that SDK path in Keil is correct relative to project location

**Problem**: Link errors about undefined references

- **Solution**: Ensure all required source files are added to project in Keil

### Flashing Issues

**Problem**: J-Link not detected

- **Solution**:
  - Check USB cable connection
  - Install/update J-Link drivers
  - Try different USB port

**Problem**: "No target connected"

- **Solution**:
  - Check SWD connections (SWDIO, SWCLK, GND, VCC)
  - Verify device has power
  - Check if device is in sleep mode (may need to enable debug during sleep)

### Runtime Issues

**Problem**: CO2 sensor not responding

- **Solution**:
  - Check I2C connections
  - Verify sensor power supply
  - Check sensor I2C address in code

**Problem**: BLE not advertising

- **Solution**:
  - Verify SoftDevice is programmed
  - Check if Bluetooth is enabled in configuration
  - Verify antenna connections

**Problem**: Display not working

- **Solution**:
  - Check SPI connections
  - Verify LCD power supply
  - Check LCD initialization sequence

### Debugging

Enable RTT (Real-Time Transfer) logging:

1. Connect J-Link debugger
2. Open J-Link RTT Viewer
3. Set Target Device: nRF52832_xxAA
4. Connect
5. View real-time logs from `NRF_LOG_*` macros

## 📚 Additional Resources

- [nRF52832 Product Specification](https://www.nordicsemi.com/Products/nRF52832)
- [nRF5 SDK Documentation](https://infocenter.nordicsemi.com/index.jsp)
- [SCD4x Sensor Datasheet](https://www.sensirion.com/en/environmental-sensors/carbon-dioxide-sensors/carbon-dioxide-sensor-scd4x/)
- [Keil µVision Documentation](http://www.keil.com/support/man/docs/uv4/)
- [Segger J-Link User Guide](https://www.segger.com/downloads/jlink/UM08001)

## 🤝 Contributing

Contributions are welcome! Please:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

### Coding Standards

- Follow existing code style
- Comment complex logic
- Update documentation for new features
- Test thoroughly before submitting

## 📄 License

This project includes:

- **Nordic nRF5 SDK**: Licensed under Nordic 5-Clause License (see SDK license files)
- **Application Code**: [Specify your license here]
- **Third-party Libraries**: See individual library licenses in their respective directories

### Third-Party Components

- **Nordic nRF5 SDK v17.1.0**: Nordic Semiconductor ASA
- **Segger RTT**: SEGGER Microcontroller GmbH
- **ARM CMSIS**: ARM Limited
- **SCD4x Driver**: Based on Sensirion sample code

---

## 🔐 Security Note

This firmware uses secure DFU with public key cryptography. The private key for signing updates is NOT included in this repository and must be kept secure. The public key in `DFU_PKG/public_key.c` is meant to be public and is safe to distribute.

## 📧 Support

For issues, questions, or contributions:

- Open an issue on GitHub
- [Add your contact information]

---

**Current Firmware Version**: 1.5.4  
**Last Updated**: November 2025  
**Supported Hardware**: AirSpot Custom nRF52832 Board
