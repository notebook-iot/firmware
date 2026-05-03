.PHONY: all setup build flash monitor menuconfig clean erase flash-secrets

HOME_DIR        := $(HOME)
ESP_IDF_EXPORT  := $(HOME)/esp/esp-idf/export.sh

COUNTRY_CODE   ?= US
TARGET_CHIP    ?= esp32s3
SERIAL_PORT    ?= /dev/ttyACM0
BAUDRATE       ?= 115200

all: build

setup:
	@bash -c '. $(ESP_IDF_EXPORT) && \
	idf.py set-target $(TARGET_CHIP)'

menuconfig:
	@bash -c '. $(ESP_IDF_EXPORT) && \
	idf.py menuconfig'

build:
	@bash -c '. $(ESP_IDF_EXPORT) && \
	idf.py build'

flash:
	@bash -c '. $(ESP_IDF_EXPORT) && \
	idf.py -p $(SERIAL_PORT) flash'

flash-secrets:
	@bash -c '. $(ESP_IDF_EXPORT) && \
	python3 $$IDF_PATH/components/nvs_flash/nvs_partition_generator/nvs_partition_gen.py generate secrets.csv secrets.bin 0x4000 && \
	esptool.py -p $(SERIAL_PORT) -b $(BAUDRATE) --before no_reset --after no_reset write_flash 0x110000 secrets.bin'

monitor:
	@bash -c '. $(ESP_IDF_EXPORT) && \
	idf.py -p $(SERIAL_PORT) -b $(BAUDRATE) monitor'

erase:
	@bash -c '. $(ESP_IDF_EXPORT) && \
	idf.py -p $(SERIAL_PORT) erase-flash'

clean:
	@bash -c '. $(ESP_IDF_EXPORT) && idf.py fullclean'
