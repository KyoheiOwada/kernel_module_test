/*
 * led.c
 * This is Device driver to handle LEDs for Raspberry Pi.
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * */

#include <linux/module.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<linux/device.h>
#include<linux/uaccess.h>
#include<linux/io.h>
#include<linux/timer.h>

MODULE_AUTHOR("Kyohei Owada");
MODULE_DESCRIPTION("driver for LED control by using timer");
MODULE_LICENSE("GPL v3");
MODULE_VERSION("0.1");

static dev_t dev;
static struct cdev cdv;
static struct class *cls = NULL;
static volatile u32 *gpio_base = NULL;
static struct timer_list mytimer;
static int interval = 500;
static int count = 0;
char let1[] = {0,1,0,1,0,1,1,1,0,1,0,0,0};
char let2[] = {1,0,1,0,1,0,1,1,1,0,0,0};
char let3[] = {1,1,1,0,1,1,1,0,1,0,1,0,1,1,1,0,0,0,1,0,1,0};

static void mytimer_fn(unsigned long arg)
{
	if(count < 13){
		if(let1[count] == 0){
			gpio_base[10] = 0x3 << 24;
		}
		else if(let1[count] == 1){
			gpio_base[7] = 0x3 << 24;
		}
	}
	else if(count >= 13 && count <25){
		if(let2[count-13] == 0){
			gpio_base[10] = 0x3 << 24;
		}
		else if(let2[count-13] == 1){
			gpio_base[7] = 0x3 << 24;
		}
	}
	else if(count >= 25 && count < 47){
		if(let3[count-25] == 0){
			gpio_base[10] = 0x3 <<24;
		}
		else if(let3[count-25] == 1){
			gpio_base[7] = 0x3 <<24;
		}
	}

	else if(count >= 47 && count < 55){
		 printk(KERN_INFO "Waiting...\n");
	}
	else if(count == 55){
	       	count = 0;
	}
	count++;
	mod_timer(&mytimer,jiffies +interval *  HZ/1000);
}

static ssize_t led_write(struct file* filp,const char* buf,size_t count,loff_t* pos)
{
	char c;
	if(copy_from_user(&c,buf,sizeof(char)))
	return -EFAULT;

	if(c == 'q'){
		gpio_base[10] = 0x3 << 24;
		del_timer(&mytimer);
	}
	else if(c == 'n'){
		add_timer(&mytimer);

	}

	return 1;
}

static struct file_operations led_fops = {
	.owner = THIS_MODULE,
	.write = led_write
};

static int __init init_mod(void)
{
	int retval;
	gpio_base = ioremap_nocache(0x3f200000,0xA0);
	
	const u32 led = 24;
	const u32 index = led/10;
	const u32 shift = (led%10)*3;
	const u32 mask = ~(0x3F << shift);


	init_timer(&mytimer);
	mytimer.expires = jiffies + HZ;
	mytimer.data = 0;
	mytimer.function = mytimer_fn;
	add_timer(&mytimer);
	gpio_base[index] = (gpio_base[index] & mask) | (0x9 << shift);

	retval = alloc_chrdev_region(&dev,0,1,"led");
	if(retval < 0){
		printk(KERN_ERR "alloc_chrdev_region failed.\n");
		return retval;
	}
	printk(KERN_INFO "%s is loaded. major:%d\n",__FILE__,MAJOR(dev));

	cdev_init(&cdv,&led_fops);
	retval = cdev_add(&cdv,dev,1);
	if(retval < 0){
		printk(KERN_ERR "cdev_add failed. major:%d, minor:%d",MAJOR(dev),MINOR(dev));
		return retval;
	}
	cls = class_create(THIS_MODULE,"led");
	if(IS_ERR(cls)){
		printk(KERN_ERR "class_create failed.");
		return PTR_ERR(cls);
	}
	device_create(cls,NULL,dev,NULL,"LEDnipple%d",MINOR(dev));

	return 0;
}

static void __exit cleanup_mod(void)
{
	del_timer(&mytimer);
	cdev_del(&cdv);
	device_destroy(cls,dev);
	class_destroy(cls);
	unregister_chrdev_region(dev,1);
	printk(KERN_INFO "%s is unloaded. major:%d\n",__FILE__,MAJOR(dev));
}

module_init(init_mod);
module_exit(cleanup_mod);
