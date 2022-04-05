#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>     	/* This is where libfs stuff is declared */
#include <asm/atomic.h>
#include <asm/uaccess.h>	/* copy_to_user */
#include <linux/slab.h>

#define FIB_MAGIC 0x1999102
#define TMPSIZE 22
#define FILES 2

static const char fib_hello_string[] = "Hello world!\n\0";
static ssize_t fib_hello_size = sizeof(fib_hello_string);

static atomic_t counters[FILES - 1];

static int fibfs_open(struct inode *inode, struct file *filp)
{
	printk(KERN_ERR "ROY: %d %s\n", __LINE__, __func__);
	if (inode->i_ino > FILES) {
		printk(KERN_ERR "ROY 111: %d %s\n", __LINE__, __func__);
		return -ENODEV;  /* Should never happen.  */
	}
	printk(KERN_ERR "ROY 222: %d %s\n", __LINE__, __func__);
	filp->private_data = counters + inode->i_ino - 1;
	printk(KERN_ERR "ROY 333: %d %s\n", __LINE__, __func__);
	return 0;
}

static unsigned long long fib_calculate_nth(int cnt) 
{	
	unsigned long long next, first = 0, second = 1;
	int i;
	
        printk(KERN_NOTICE "fibfs-driver: Clculating fib number %d\n", cnt);
	if (cnt == 0)
		return first;
	if (cnt == 1)
		return second;

	next = first + second;	
	for (i = 3; i <= cnt; i++){
		first = second;
		second = next;
		next = first + second;	
	}

	return next;
}
/*
 *  * Read a file.  Here we increment and read the counter, then pass it
 *   * back to the caller.  The increment only happens if the read is done
 *    * at the beginning of the file (offset = 0); otherwise we end up counting
 *     * by twos.
 *      */
static ssize_t fibfs_read_file(struct file *filp, char *buf,
		size_t count, loff_t *offset)
{
	int v, len, err;
	unsigned long long fib;
	char tmp[TMPSIZE];
	atomic_t *counter = (atomic_t *) filp->private_data;
	/*
	 *  * Encode the value, and figure out how much of it we can pass back.
	 *   */
        printk(KERN_NOTICE "fibfs-driver: Device file is read at offset = %i, read bytes count = %u\n"
                        , (int)*offset, (unsigned int)count );
	//tmp = (char *)kzalloc(sizeof(int), GFP_KERNEL);
	v = atomic_read(counter);
	if (*offset > 0)
		v -= 1;  /* the value returned when offset was zero */
	else
		atomic_inc(counter);
	fib = fib_calculate_nth(v);
	len = snprintf(tmp, TMPSIZE, "%lld\n", fib);
	if (*offset > len) {
                printk(KERN_NOTICE "fibfs-drive: Nothing to read\n");
		err = 0;
		goto out;
	}
	if (count > len - *offset) {
                printk(KERN_NOTICE "fibfs-driver: Read partial string\n");
		count = len - *offset;
	}
	/*
	 *  * Copy it back, increment the offset, and we're done.
	 *   */
	if (copy_to_user(buf, tmp + *offset, count)) {
                printk(KERN_ERR "fibfs-driver: copy_to_user failed\n");
		err = -EFAULT;
		goto out;
	}
	*offset += count;
	err = count;

out:
	return err;
}

static ssize_t fibfs_hello_read_file(struct file *filp, char *buf,
                size_t count, loff_t *offset)
{       
        printk(KERN_NOTICE "fibfs-driver hello: Device file is read at offset = %i, read bytes count = %u\n"
                        , (int)*offset, (unsigned int)count );
        /* If offset is behind the end of a file we have nothing to read */
        if( *offset >= fib_hello_size ) {
                printk(KERN_NOTICE "fibfs-drive hello: Nothing to read\n");
                return 0;
        }
        /* If a user tries to read more than we have, read only as many bytes as we have */
        if( *offset + count > fib_hello_size ) {
                printk(KERN_NOTICE "fibfs-driver hello: Read partial string\n");
                count = fib_hello_size - *offset;
        }
        if( copy_to_user(buf, fib_hello_string + *offset, count) != 0 ) {
                printk(KERN_ERR "fibfs-driver hello: copy_to_user failed\n");
                return -EFAULT;
        }
        /* Move reading offset */
        *offset += count;
                return count;
}

/*
 *  * Now we can put together our file operations structure.
 *   */
static struct file_operations fibfs_file_ops = {
	.open	= fibfs_open,
	.read 	= fibfs_read_file,
};

static struct file_operations fibfs_hello_file_ops = {
	.open	= fibfs_open,
	.read 	= fibfs_hello_read_file,
};



/*
 *  * OK, create the files that we export.
 *   */
struct tree_descr fib_files[] = {
	{ NULL, NULL, 0 },  /* Skipped */
	{ .name = "hello.txt",
		.ops = &fibfs_hello_file_ops,
		.mode = S_IRUGO },
	{ .name = "fib.num",
		.ops = &fibfs_file_ops,
		.mode = S_IWUSR|S_IRUGO },
	{ "", NULL, 0 }
};



/*
 *  * Superblock stuff.  This is all boilerplate to give the vfs something
 *   * that looks like a filesystem to work with.
 *    */

/*
 *  * "Fill" a superblock with mundane stuff.
 *   */
static int fibfs_fill_super (struct super_block *sb, void *data, int silent)
{
	int err;

	printk(KERN_ERR "ROY: %d %s\n", __LINE__, __func__);
	err = simple_fill_super(sb, FIB_MAGIC, fib_files);
	if (err) {
		printk(KERN_ERR "ROY: %d %s | failed to fill super\n", __LINE__, __func__);
	}
	return err;
}


/*
 *  * Stuff to pass in when registering the filesystem.
 *   */
static struct dentry *fibfs_get_super(struct file_system_type *fst,
		int flags, const char *devname, void *data)
{
	struct dentry *entry;

	printk(KERN_ERR "ROY: %d %s\n", __LINE__, __func__);
	entry =  mount_nodev(fst, flags, data, fibfs_fill_super);
	if (!entry) {
		printk(KERN_ERR "ROY: %d %s | failed to mount_nodev\n", __LINE__, __func__);
	}
	return entry;
}

static struct file_system_type fibfs_type = {
	.owner 		= THIS_MODULE,
	.name		= "fibfs",
	.mount		= fibfs_get_super,
	.kill_sb	= kill_litter_super,
};

/*
 *  * Get things set up.
 *   */
static int __init fibfs_init(void)
{
	int err, i;
	
	printk(KERN_ERR "ROY: %d %s | HELLO!\n", __LINE__, __func__);
	for (i = 0; i < FILES; i++)
		atomic_set(counters + i, 0);
	err = register_filesystem(&fibfs_type);
	if (err) {
		printk(KERN_ERR "ROY: %d %s | failed to register filesystem\n", __LINE__, __func__);
	}
	return err;
}

static void __exit fibfs_exit(void)
{
	printk(KERN_ERR "ROY: %d %s | GOODBYE!\n", __LINE__, __func__);
	unregister_filesystem(&fibfs_type);
}

module_init(fibfs_init);
module_exit(fibfs_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Roy Novich");
