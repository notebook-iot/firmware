.PHONY: all setup build flash monitor menuconfig clean erase

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

monitor:
	@bash -c '. $(ESP_IDF_EXPORT) && \
	idf.py -p $(SERIAL_PORT) -b $(BAUDRATE) monitor'

erase:
	@bash -c '. $(ESP_IDF_EXPORT) && \
	idf.py -p $(SERIAL_PORT) erase-flash'

clean:
	@bash -c '. $(ESP_IDF_EXPORT) && idf.py fullclean'
