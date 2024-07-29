TOP_DIR = /media/user1/V3SSDK_BUILDROOT/v3ssdk/buildroot/out/
CROSS = $(TOP_DIR)host/bin/arm-buildroot-linux-gnueabihf-

CC = $(CROSS)gcc
LIBMODBUS_PATH=modbus

ptc310_app:
	$(CC) -D__UNIX__ Protocol/*.c Protocol/Modbus/*.c *.c \
                -Wall -O2 -I$(LIBMODBUS_PATH)/ -I. \
                -IProtocol/ \
                -IProtocol/Modbus/ \
                -L$(LIBMODBUS_PATH)/ -lmodbus -lpthread -lm -o ptc310_app 
clean:
	rm ptc310_app 
