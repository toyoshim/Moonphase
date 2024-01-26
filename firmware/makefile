DEFS	 = -D_HID_NO_PS3 -D_HID_NO_GUNCON3 -D_HID_NO_KEYBOARD -D_HID_NO_SWITCH \
	-D_HID_NO_XBOX
CFLAGS = -V -mmcs51 --model-large --xram-size 0x1800 --xram-loc 0x0000 \
	--code-size 0xec00 --stack-auto --Werror -Ichlib/src --opt-code-speed \
	$(DEFS)
CC     = sdcc
TARGET = moonphase
OBJS  = main.rel \
  ch559.rel flash.rel gpio.rel hid.rel hid_mouse.rel led.rel serial.rel \
	timer3.rel uart1.rel usb_host.rel

.PHONY: all program run clean build

all: build $(TARGET).bin

program: $(TARGET).bin
	ch559flasher -e -w $<

run: program
	ch559flasher -b

clean:
	rm -rf build $(TARGET).bin

.SILENT:
build:
	mkdir -p build

build/%.rel: %.c chlib/src/*.h chlib/src/usb/*.h chlib/src/usb/hid/*.h
	$(CC) -c $(CFLAGS) -o $@ $<

build/%.rel: chlib/src/%.c chlib/src/*.h chlib/src/usb/*.h chlib/src/usb/hid/*.h
	$(CC) -c $(CFLAGS) -o $@ $<

build/%.rel: chlib/src/usb/%.c chlib/src/*.h chlib/src/usb/*.h chlib/src/usb/hid/*.h
	$(CC) -c $(CFLAGS) -o $@ $<

build/%.rel: chlib/src/usb/hid/%.c chlib/src/*.h chlib/src/usb/*.h chlib/src/usb/hid/*.h
	$(CC) -c $(CFLAGS) -o $@ $<

build/$(TARGET).ihx: $(addprefix build/,$(OBJS))
	$(CC) $(CFLAGS) $^ -o $@

%.bin: build/%.ihx
	sdobjcopy -I ihex -O binary $< $@
