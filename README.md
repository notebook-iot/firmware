# firmware
Firmware for board

# Flashing using a USB-UART bridge

1. Put the board into bootloader mode by holding boot on power up, or:
    - Hold boot
    - Press reset/EN
    - Release boot
2. Flash `secrets.csv` (optional)

```
make flash secrets
```

3. Flash directly using `esptool`:

```
esptool.py --chip esp32s3 --port /dev/ttyACM0 --baud 115200 --before no_reset --after no_reset write_flash --flash_mode dio --flash_freq 80m --flash_size 2MB 0x0 build/bootloader/bootloader.bin 0x8000 build/partition_table/partition-table.bin 0x10000 build/page.bin
```
