all:
	gcc -Wall ssd_fuse.c `pkg-config fuse3 --cflags --libs` -D_FILE_OFFSET_BITS=64 -o ssd_fuse
	gcc -Wall ssd_fuse_dut.c -o ssd_fuse_dut
run:
	./ssd_fuse -d /tmp/ssd
hello_world:
	echo "hello world" > /tmp/ssd/ssd_file
	cat /tmp/ssd/ssd_file
test1:
	sh test.sh test1
test2:
	sh test.sh test2
test3:
	sh test.sh test3
test4:
	sh test.sh test4
write_test:
	./ssd_fuse_dut /tmp/ssd/ssd_file w 200 10
read_test:
	./ssd_fuse_dut /tmp/ssd/ssd_file r 200 10
physical_size:
	./ssd_fuse_dut /tmp/ssd/ssd_file p
logical_size:
	./ssd_fuse_dut /tmp/ssd/ssd_file l
umount:
	#fusermount -u /tmp/ssd
	umount /tmp/ssd
clean:
	rm debug.txt
	rm ssd_fuse ssd_fuse_dut
	rm -rf /tmp/ssd
	mkdir /tmp/ssd
	rm -rf ssd
	mkdir ssd
