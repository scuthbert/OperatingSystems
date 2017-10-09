#include<linux/module.h>

#include<linux/fs.h>
#include<linux/slab.h>
#include<asm/uaccess.h>

#define BUFFER_SIZE 1024

/* Define device_buffer and other global data structures you will need here */
int openCount;
int closeCount;

int locationInBuffer;

char *device_buffer;

ssize_t simple_char_driver_read (struct file *pfile, char __user *buffer, size_t length, loff_t *offset)
{
	/* *buffer is the userspace buffer to where you are writing the data you want to be read from the device file*/
	/* length is the length of the userspace buffer*/
	/* offset will be set to current position of the opened file after read*/
	/* copy_to_user function: source is device_buffer and destination is the userspace buffer *buffer */
	
	if(! *offset) { // First Read
		*offset = 0; // Let's read from the start
	}

	copy_to_user(buffer, device_buffer + *offset, length);

	printk(KERN_ALERT "Simple Char Driver: Just read %lu bytes starting at %lld into the buffer.", length, *offset);

	return 0;
}



ssize_t simple_char_driver_write (struct file *pfile, const char __user *buffer, size_t length, loff_t *offset)
{
	/* *buffer is the userspace buffer where you are writing the data you want to be written in the device file*/
	/* length is the length of the userspace buffer*/
	/* current position of the opened file*/
	/* copy_from_user function: destination is device_buffer and source is the userspace buffer *buffer */

	return length;
}


int simple_char_driver_open (struct inode *pinode, struct file *pfile)
{
	/* print to the log file that the device is opened and also print the number of times this device has been opened until now*/
	openCount++;
	printk(KERN_ALERT "Simple Char Driver: Open #%d\n", openCount);
	return 0;
}

int simple_char_driver_close (struct inode *pinode, struct file *pfile)
{
	/* print to the log file that the device is closed and also print the number of times this device has been closed until now*/
	closeCount++;
	printk(KERN_ALERT "Simple Char Driver: Close #%d\n", closeCount);
	return 0;
}

loff_t simple_char_driver_seek (struct file *pfile, loff_t offset, int whence)
{
	/* Update open file position according to the values of offset and whence */
	return 0;
}

struct file_operations simple_char_driver_file_operations = {

	.owner   = THIS_MODULE,
	.open = simple_char_driver_open,
	.release = simple_char_driver_close,
	.llseek = simple_char_driver_seek,
	.write = simple_char_driver_write,
	.read = simple_char_driver_read

};

static int simple_char_driver_init(void)
{
	/* print to the log file that the init function is called.*/
	printk(KERN_ALERT "Simple Char Driver: In Init\n");
	/* register the device */
	register_chrdev(240, "simple_character_device", &simple_char_driver_file_operations);
	openCount = 0;
	closeCount = 0;
	locationInBuffer = 0;	

	device_buffer = kmalloc(BUFFER_SIZE, GFP_KERNEL);

	return 0;
}

void simple_char_driver_exit(void)
{
	/* print to the log file that the exit function is called.*/
	printk(KERN_ALERT "Simple Char Driver: In Exit\n");
	/* unregister  the device using the register_chrdev() function. */
	unregister_chrdev(240, "simple_character_device");
	kfree(device_buffer);
}

/* addegistered protocol family 40
 to point to the corresponding init and exit function*/
module_init(simple_char_driver_init);
module_exit(simple_char_driver_exit);
