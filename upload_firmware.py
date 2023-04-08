
import os
import time

PORT = ""
# PORT = "--port /dev/cu.usbserial-00000000"
# PORT = "--port /dev/cu.usbserial-A800H7OY"

os.system('"/Users/matejsuchanek/.platformio/penv/bin/python" "/Users/matejsuchanek/.platformio/packages/tool-esptoolpy/esptool.py" {PORT} --chip esp32 --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size 4MB 0x1000 /Users/matejsuchanek/Documents/Spectoda/Spectoda_Firmware/.pio/build/spectoda/bootloader.bin 0x8000 /Users/matejsuchanek/Documents/Spectoda/Spectoda_Firmware/.pio/build/spectoda/partitions.bin 0xe000 /Users/matejsuchanek/Documents/Spectoda/Spectoda_Firmware/.pio/build/spectoda/ota_data_initial.bin 0x10000 .pio/build/spectoda/firmware.bin'.format(PORT = PORT))

while(True):
    time.sleep(3)
    os.system('"/Users/matejsuchanek/.platformio/penv/bin/python" "/Users/matejsuchanek/.platformio/packages/tool-esptoolpy/esptool.py" {PORT} --chip esp32 --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size 4MB 0x1000 /Users/matejsuchanek/Documents/Spectoda/Spectoda_Firmware/.pio/build/spectoda/bootloader.bin 0x8000 /Users/matejsuchanek/Documents/Spectoda/Spectoda_Firmware/.pio/build/spectoda/partitions.bin 0xe000 /Users/matejsuchanek/Documents/Spectoda/Spectoda_Firmware/.pio/build/spectoda/ota_data_initial.bin 0x10000 .pio/build/spectoda/firmware.bin'.format(PORT = PORT))