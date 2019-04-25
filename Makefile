obj-m:= led.o

myled.ko: led.c
	make -C /usr/src/linux M='/home/pi/workspace' V=1 modules
clean:
	make -C /usr/src/linux M='/home/pi/workspace' V=1 clean
