/*
 * @Author: kaisen
 * @Date: 2020-02-13 15:22:38
 * @LastEditTime: 2020-03-07 13:48:39
 * @LastEditors: KaisenSir
 * @Description: lte-link button driver source file
 * @FilePath: /lte_btnDriver/lte_btnDriver.c
 */
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/version.h>
#include <linux/pagemap.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/poll.h>
#include <linux/sched.h>

#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/gpio.h>

#include "lte_btnDriver.h"

static DECLARE_WAIT_QUEUE_HEAD(button_waitq);
static volatile int ev_press = 0;   //中断事件标志, 中断服务程序将它置1，btn_drv_read将它清0 
static struct fasync_struct *button_async;

static unsigned char key_val;

struct key_desc {                 
    unsigned int key_ionum_chip;        // key Gpio chip num
    unsigned int key_ionum_bits;        // key Gpio offset num
    unsigned int key_ionum;             // key Gpio num
	unsigned int key_val;		        // key value 
	unsigned int number;		        // key serial number
	char *name;			                // key name 
};

struct key_desc keys_descs[] = {     //gpio informations
	{ GPIO_CHIP_10, GPIO_BITS_6, 0, 0, 0, "KEY_0" },
};

/*
 * interrupt types
 * 0 - disable irq
 * 1 - rising edge triggered
 * 2 - falling edge triggered
 * 3 - rising and falling edge triggered
 * 4 - high level triggered
 * 8 - low level triggered
 */
// static unsigned int gpio_irq_type = 0;
static unsigned int gpio_irq_type = 3;
module_param(gpio_irq_type, uint, S_IRUGO);
MODULE_PARM_DESC(gpio_irq_type, "gpio irq type");
spinlock_t lock;

/*********************************************************************************************************
 * @description: open button driver interface
 * @param ...
 * @return: ...
 **********************************************************************************************************/
int btn_drv_open(struct inode * inode, struct file * file)
{
    return 0;
}

/*********************************************************************************************************
 * @description: close button driver interface
 * @param ...
 * @return: ...
 **********************************************************************************************************/
int btn_drv_close(struct inode * inode, struct file * file)
{
    return 0;
}

/*********************************************************************************************************
 * @description: Module software round robin
 * @param ...
 * @return: ...
 **********************************************************************************************************/
static unsigned btn_drv_poll(struct file *file, poll_table *wait)
{
	unsigned int mask = 0;
	poll_wait(file, &button_waitq, wait); // 不会立即休眠，只是把进程挂到队列里面去

	if (ev_press)                          //判断是否有数据返回。有的话进行赋值，没有的话休眠
		mask |= POLLIN | POLLRDNORM;    //返回位掩码, 它描述哪个操作可马上被实现。

	return mask;
}

/*********************************************************************************************************
 * @description: Asynchronous notification
 * @param ...
 * @return: ...
 **********************************************************************************************************/
static int btn_drv_fasync (int fd, struct file *filp, int on)   
{
	// printk("driver: btn_drv_fasync\n");                 //为了说明次函数被调用增加一条打印语句
	return fasync_helper (fd, filp, on, &button_async);     //初始化定义的结构体
}

/*********************************************************************************************************
 * @description: read key value
 * @param ...
 * @return: IRQ_HANDLED
 **********************************************************************************************************/
static int btn_drv_read(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
    if(file->f_flags & O_NONBLOCK)
    {         
		copy_to_user(buf, &key_val, 1); /* non-blocking */
	}
    else
    {
		wait_event_interruptible(button_waitq, ev_press);	//if no action of key it will sleep
		copy_to_user(buf, &key_val, 1);     // return key value 
		ev_press = 0;
	}

    return 0;
}

/*********************************************************************************************************
 * @description: interrupt handler 
 * @irq     interrrupt number 
 * @dev_id  device id
 * @return: IRQ_HANDLED
 **********************************************************************************************************/
static irqreturn_t buttons_irq_handler(int irq, void *dev_id)
{
    key_val = gpio_get_value(keys_descs[0].key_ionum);
    // printk("buttons IRQ handler key_val = %d\n", key_val);
    mdelay(20);
    ev_press = 1;
    wake_up_interruptible(&button_waitq);
    kill_fasync(&button_async, SIGIO, POLL_IN);

    return IRQ_HANDLED;
}

/*********************************************************************************************************
 * @description: registrantion interrupt configure
 * @param none
 * @return: status
 **********************************************************************************************************/
static int gpio_dev_irq_init(unsigned int ionum)
{
    unsigned int irq_num;
    unsigned int irqflags = 0;
    
    // set gpio mode to "input"
    if (gpio_direction_input(ionum)) 
    {
        pr_err("set gpio num to inpuut mode fail!\n");
        return -EIO;
    }
    
    // switch gpio interrupt types
    switch (gpio_irq_type) 
    {
    case 1:
        irqflags = IRQF_TRIGGER_RISING;
        break;
    case 2:
        irqflags = IRQF_TRIGGER_FALLING;
        break;
    case 3:
        irqflags = IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING;
        break;
    case 4:
        irqflags = IRQF_TRIGGER_HIGH;
        break;
    case 8:
        irqflags = IRQF_TRIGGER_LOW;
        break;
    default:
        pr_info("gpio_irq_type error!\n");
        return -1;
    }    
    irqflags |= IRQF_SHARED;

    // Mapping interrupt numbers based on GPIO numbers
    irq_num = gpio_to_irq(ionum);
    
    // Registration interrupted
    if (request_irq(irq_num, buttons_irq_handler, irqflags, "gpio interrupt register", &gpio_irq_type)) {
        pr_info("irq reuqest error!\n");
        return -1;
    }
    
    return 0;
}

/*********************************************************************************************************
 * @description: release registrantion interrupt configure
 * @param ionum     Gpio number
 * @return: none
 **********************************************************************************************************/
static void gpio_dev_irq_exit(unsigned int ionum)
{
    unsigned long flags;

    // Release registered interrupt
    spin_lock_irqsave(&lock, flags);
    free_irq(gpio_to_irq(ionum), &gpio_irq_type);
    spin_unlock_irqrestore(&lock, flags);
}

static struct file_operations btn_drv_fops = {
    .owner      = THIS_MODULE,
    .open       = btn_drv_open,
    .read       = btn_drv_read,
    .release    = btn_drv_close,
    .poll       = btn_drv_poll,	    //用户程序使用select调用的时候才会用到poll
    .fasync	    = btn_drv_fasync	//用户程序用异步通知的时候才会用到fasync
};

static struct miscdevice btn_drv_dev = {
   .minor		= MISC_DYNAMIC_MINOR,
   .name		= "buttons",
   .fops        = &btn_drv_fops,
};

static int __init btn_driver_init(void)
{
    int status = 0;

    if (misc_register(&btn_drv_dev))
    {
        pr_err("insmod button driver fail [misc_register]\n");
        return -1;
    }

    spin_lock_init(&lock);
    keys_descs[0].key_ionum = keys_descs[0].key_ionum_chip * 8 + keys_descs[0].key_ionum_bits;

    // registration interrupted of gpio_num
    if (gpio_request(keys_descs[0].key_ionum, NULL)) {
        pr_err("Gpio request fail! io number=%d \n", keys_descs[0].key_ionum);
        return -EIO;
    }

    if ((status = gpio_dev_irq_init(keys_descs[0].key_ionum)))
        gpio_free(keys_descs[0].key_ionum);
    else
        printk(KERN_INFO "load btn_driver.ko ...OK!\n");
	
    return status;
}

static void __exit btn_driver_exit(void)
{
    gpio_dev_irq_exit(keys_descs[0].key_ionum);
    gpio_free(keys_descs[0].key_ionum);

    printk(KERN_INFO "unload btn_driver.ko ...OK!\n");
	
    return;
}

module_init(btn_driver_init);
module_exit(btn_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("kaisen@cuav.net");

