TOP_DIR = /media/user1/V3SSDK_BUILDROOT/v3ssdk/buildroot/out/
CROSS = $(TOP_DIR)host/bin/arm-buildroot-linux-gnueabihf-

CC = $(CROSS)gcc
LIBMODBUS_PATH=modbus

config_editor:
	$(CC) -D__UNIX__ *.c \
                -Wall -O2 -I$(LIBMODBUS_PATH)/ -I. \
                -L$(LIBMODBUS_PATH)/ -lpthread -lm -o ptc310_config_editor 
clean:
	rm ptc310_config_editor 
