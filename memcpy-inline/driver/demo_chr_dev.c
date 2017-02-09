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
} km_data_t;

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
	km_data_t *p;
	pr_info("++chr_read, g_flag = %d\n", g_flag);

	p = kzalloc(sizeof(km_data_t), GFP_KERNEL);
	if (!p) {
		pr_err("kmalloc failed\n");
		return -ENOMEM;
	}

	pr_info("loop #: %d\n", loop);
	j0 = jiffies;
	for (i = 0; i < loop; i++) {
		if (g_flag == 1) // const copy size
			n = copy_from_user(p, u, sizeof(km_data_t));
		else
			n = copy_from_user(p, u, sz);
	}
	j1 = jiffies;

	pr_info("n = %lu, j0 = %lu, j1 = %lu\n", n, j0, j1);
	pr_info("a = 0x%lx, b = 0x%lx\n", p->a, p->b); 	
	kfree(p);

	return n;
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
