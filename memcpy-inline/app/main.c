#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/ioctl.h>

#define CHRDEV  0xCD
#define CHR_SET_FLAG    _IO(CHRDEV, 0x30)

#define CHR_DEV_NAME "/dev/chr_dev"

typedef struct {
	unsigned long a;
	unsigned long b;
	unsigned long c;
	unsigned long d;
} um_t;

int main(int argc, char *argv[])
{
	int ret, flag = 0;
	um_t um;

	if (argc != 2) {
		printf("Usage:\n main {0, 1}(=1, inline; =0, non-inline)\n");
		return -1;
	}

	flag = atoi(argv[1]);
	printf("flag = %d\n", flag);

	um.a = 0xdeadbeefbeefdead;
	um.b = 0xabcdabcdabcdabcd;
	um.c = 0xcccccccccccccccc;
	um.d = 0xdddddddddddddddd;	
	int fd = open(CHR_DEV_NAME, O_RDONLY|O_NDELAY);
	if(fd < 0) {
        	printf("open file %s failed!\n", CHR_DEV_NAME);
		return -1;
	}
	ret = ioctl(fd, CHR_SET_FLAG, &flag);
	if (ret < 0) {
		printf("ioctl failed\n");
		return -1;
	} 

	ret = read(fd, &um , sizeof(um_t));
	printf("ret = %d, um.a = 0x%lx, um.b = 0x%lx\n", ret, um.a, um.b);
	if (ret < 0) {
		printf("read file %s failed!\n", CHR_DEV_NAME);
		return -1;
	}

	close(fd);

	return 0;
}

