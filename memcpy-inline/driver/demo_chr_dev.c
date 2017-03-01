#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#define CHRDEV	0xCD
#define CHR_SET_FLAG	_IO(CHRDEV, 0x30)

/* g_flag = 1 will trigger the inline optimization */
static int g_flag;
/* loop count of copy_{from, to}_user */
static int loop = 10000; 

static struct cdev chr_dev;
static dev_t ndev;

typedef struct {
	unsigned long a;
	unsigned long b;
} km_data_16;

typedef struct {
	unsigned long a;
	unsigned long b;
	unsigned long c;
} km_data_24;

typedef struct {
	unsigned long a;
	unsigned long b;
	unsigned long c;
	unsigned long d;
} km_data_32;

static int chr_open(struct inode *nd, struct file *filp)
{
	int major = MAJOR(nd->i_rdev);
	int minor = MINOR(nd->i_rdev);
	pr_info("chr_open, major=%d, minor=%d\n", major, minor);
	return 0;
}

static ssize_t chr_read(struct file *filp, char __user *u, size_t sz, loff_t *off)
{
	unsigned long n, j0, j1, i;
	unsigned char v1 = 0xcd;
	unsigned short v2 = 0xabef;
	unsigned int v4 = 0xdeadbeef;
	unsigned long v8 = 0xdead12345678beef;
	km_data_16 *p_16;
	km_data_24 *p_24;
	km_data_32 *p_32;
	size_t sz_1, sz_2, sz_4, sz_8, sz_16, sz_24, sz_32;
	int ret = 0;
	
	pr_info("++chr_read, g_flag = %d\n", g_flag);

	sz_1 = sz - 31;
	sz_2 = sz - 30;
	sz_4 = sz - 28;
	sz_8 = sz - 24;
	sz_16 = sz - 16;
	sz_24 = sz - 8;
	sz_32 = sz;

	p_16 = kzalloc(sizeof(km_data_16), GFP_KERNEL);
	if (!p_16) {
		pr_err("kmalloc failed\n");
		return -ENOMEM;
	}

	
	p_24 = kzalloc(sizeof(km_data_24), GFP_KERNEL);
	if (!p_24) {
		pr_err("kmalloc failed\n");
		return -ENOMEM;
	}

	p_32 = kzalloc(sizeof(km_data_32), GFP_KERNEL);
	if (!p_32) {
		pr_err("kmalloc failed\n");
		return -ENOMEM;
	}

	pr_info("loop #: %d\n", loop);
	j0 = jiffies;
	for (i = 0; i < loop; i++) {
		if (g_flag == 1) { // const copy size
			n = copy_from_user(&v1, u, sizeof(unsigned char));
			n = copy_from_user(&v2, u, sizeof(unsigned short));
			n = copy_from_user(&v4, u, sizeof(unsigned int));
			n = copy_from_user(&v8, u, sizeof(unsigned long));
			n = copy_from_user(p_16, u, sizeof(km_data_16));
			n = copy_from_user(p_24, u, sizeof(km_data_24));
			n = copy_from_user(p_32, u, sizeof(km_data_32));
			v1 += 0x10;
			v2 += 0x1000;
			v4 += 0x10000000;
			v8 += 0x1000000010000000;
			p_16->a = 0x12345678abcedfdd;
			p_16->b = 0x1234567887654321;
			p_24->c = 0x1001001001001000;
			p_32->d = 0x8765432112345678;
			n = copy_to_user(u, &v1, sizeof(unsigned char));
			n = copy_to_user(u, &v2, sizeof(unsigned short));
			n = copy_to_user(u, &v4, sizeof(unsigned int));
			n = copy_to_user(u, &v8, sizeof(unsigned long));
			n = copy_to_user(u, p_16, sizeof(km_data_16));
			n = copy_to_user(u, p_24, sizeof(km_data_24));
			n = copy_to_user(u, p_32, sizeof(km_data_32));
		}
		else {
		
			n = copy_from_user(&v1, u, sz_1);
			n = copy_from_user(&v2, u, sz_2);
			n = copy_from_user(&v4, u, sz_4);
			n = copy_from_user(&v8, u, sz_8);
			n = copy_from_user(p_16, u, sz_16);
			n = copy_from_user(p_24, u, sz_24);
			n = copy_from_user(p_32, u, sz_32);
			v1 += 0x10;
			v2 += 0x1000;
			v4 += 0x10000000;
			v8 += 0x1000000010000000;
			p_16->a = 0x12345678abcedfdd;
			p_16->b = 0x1234567887654321;
			p_24->c = 0x1001001001001000;
			p_32->d = 0x8765432112345678;
			n = copy_to_user(u, &v1, sz_1);
			n = copy_to_user(u, &v2, sz_2);
			n = copy_to_user(u, &v4, sz_4);
			n = copy_to_user(u, &v8, sz_8);
			n = copy_to_user(u, p_16, sz_16);
			n = copy_to_user(u, p_24, sz_24);
			n = copy_to_user(u, p_32, sz_32);
		}
	}
	j1 = jiffies;

	pr_info("n = %lu, j0 = %lu, j1 = %lu\n", n, j0, j1);
	kfree(p_16);
	kfree(p_24);
	kfree(p_32);

	return ret;
}

static int chr_release(struct inode *nd, struct file *filp)
{
	printk("++chr_release()!\n");
	return 0;       
}


static long chr_ioctl(struct file *filp, unsigned int ioctl, unsigned long arg)
{
	if (ioctl == CHR_SET_FLAG) {
		if (get_user(g_flag, (int __user *)arg)) {
			pr_err("get_user() failed\n");
			return -EFAULT;
		}
		pr_info("g_flag = %d\n", g_flag);
	}
	else {
		pr_err("Invalid IOCTL cmd\n");
		return -EINVAL;
	}
	return 0;
}

struct file_operations chr_ops =
{
	.owner = THIS_MODULE,
	.open = chr_open,
	.read = chr_read,
	.unlocked_ioctl = chr_ioctl,
	.release = chr_release,
};

static int demo_init(void)
{
	int ret;
	
	pr_info("++demo_init()\n");
	/* initialize the character device instance */
	cdev_init(&chr_dev, &chr_ops);
	/* allocate the device node number dynamically */
	ret = alloc_chrdev_region(&ndev, 0, 1, "chr_dev");
	if(ret < 0)
		return ret;
	pr_info("demo_init():major=%d, minor=%d\n", MAJOR(ndev), MINOR(ndev));

	/* register the char_dev into the system */
	ret = cdev_add(&chr_dev, ndev, 1);
	if(ret < 0)
		return ret;

    return 0;
}

static void demo_exit(void)
{
	pr_info("--demo_exit()\n");
	/* unregister the char_dev from the system */
	cdev_del(&chr_dev);
	/* free the device node number */
	unregister_chrdev_region(ndev, 1);
}

module_init(demo_init);
module_exit(demo_exit);

module_param(loop, int, 0);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("dennis.chen@arm.com");
MODULE_DESCRIPTION("A char device driver to demo copy_from_user opt");
