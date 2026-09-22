obj-m :=
obj-m += misc-modules/
obj-m += scull/

misc-modules-y := hello.o faulty.o seq.o jiq.o jit.o silly.o sleepy.o kdataalign.o kdatasize.o complete.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	mkdir -p module/misc-modules
	mkdir -p module/scull
	cp misc-modules/*.ko module/misc-modules/
	cp scull/*.ko module/scull/

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
