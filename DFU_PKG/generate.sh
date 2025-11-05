#!/bin/bash

# Copy commands
cp ../nRF5_SDK_17.1.0_ddde560/examples/ble_peripheral/ble_app_uart_minico2/pca10040/s112/arm5_no_packs/_build/nrf52832_xxaa.hex nrf52832_xxaa.hex
cp ../nRF5_SDK_17.1.0_ddde560/examples/dfu/secure_bootloader/pca10040e_s112_ble/arm5_no_packs/_build/nrf52810_xxaa_s112.hex nrf52810_xxaa_s112.hex
# cp ../nRF5_SDK_17.1.0_ddde560/components/softdevice/s112/hex/s112_nrf52_7.2.0_softdevice.hex s112_nrf52_7.2.0_softdevice.hex

# Generate bootloader settings
nrfutil settings generate --family NRF52 --application nrf52832_xxaa.hex --application-version 1 --bootloader-version 1 --bl-settings-version 1 bootloader_settings.hex

# Merge hex files
mergehex --merge s112_nrf52_7.2.0_softdevice.hex nrf52832_xxaa.hex --output temp1.hex
mergehex --merge temp1.hex nrf52810_xxaa_s112.hex --output temp2.hex
mergehex --merge temp2.hex bootloader_settings.hex --output final.hex

# Generate DFU package
nrfutil pkg generate --application nrf52832_xxaa.hex --application-version 0xFF --hw-version 52 --sd-req 0x103 --key-file private.pem nrf52832_xxaa.zip

# Optional commands for programming (commented out)
# ./nrfjprog -f NRF52 --eraseall
# ./nrfjprog -f NRF52 --program "final.hex" --verify
# ./nrfjprog -f NRF52 --reset

# Pause (not needed in macOS, so you can use a read command to simulate a pause if required)
echo "Script completed. Press Enter to exit."
read -r
