#include <linux/module.h>       //所有模块都需要的头文件
#include <linux/kernel.h>
#include <linux/fs.h>           //文件系统有关的，结构体file_operations也在fs头文件定义
#include <linux/irq.h>
#include <asm/uaccess.h>        //linux中的用户态内存交互函数，copy_from_user(),copy_to_user()等
#include <asm/irq.h>            //linux中断定义
#include <asm/io.h>
#include <linux/sched.h>        //声明printk()这个内核态的函数
#include <linux/interrupt.h>    //包含与中断相关的大部分宏及结构体的定义，request_irq()等
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/poll.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <linux/timer.h>

struct button_desc {
        int number;
        unsigned int value;
        char name[32];
};

struct button_desc *btn_desc = NULL;
static int btn_irq = -1;
static int btn_gpio = -1;

static int major;
static int minor;

static struct class *mclass = NULL;
static struct cdev *mcdev = NULL;

static DECLARE_WAIT_QUEUE_HEAD(button_waitq);

static volatile int ev_press = 0;
static struct fasync_struct *btnfasync;

// kernel timer
static struct timer_list button_timer;

// --------- timer interrupt -----------
static void button_timer_handler(struct timer_list *l)
{
        if (!btn_desc)
                return ;

        btn_desc->value = gpio_get_value(btn_gpio);

        ev_press = 1;
        wake_up_interruptible(&button_waitq);

        kill_fasync(&btnfasync, SIGIO, POLL_IN);
}

// -------- button interrupt -----------
static irqreturn_t button_irq_handler(int irq, void *dev_id)
{
        // struct button_desc *pbdesc = dev_id;

        mod_timer(&button_timer, jiffies + (HZ)/100);

        return IRQ_RETVAL(IRQ_HANDLED);
}

// --------- file operations ------------
static int button_open(struct inode *inode, struct file *file)
{
        return 0;
}

static int button_release(struct inode *inode, struct file *file)
{
        return 0;
}

static ssize_t button_read(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
        int ret;

        if (file->f_flags & O_NONBLOCK)
        {
                if (!ev_press)  // 没有按键动作发生
                        return -EBUSY;
        }
        else
        {
                wait_event_interruptible(button_waitq, ev_press);
                ev_press = 0;
        }

        ret = copy_to_user(buf, &btn_desc->value, sizeof(btn_desc->value));

        return ret;
}

static unsigned button_poll(struct file *file, poll_table *wait)
{
        return 0;
}

static int button_fasync(int fd, struct file *filp, int on)
{
        return fasync_helper(fd, filp, on, &btnfasync);
}

static struct file_operations button_fops = {
        .owner                  = THIS_MODULE,
        .open                   = button_open,
        .release                = button_release,
        .read                   = button_read,
        .poll                   = button_poll,
        .fasync                 = button_fasync,
};

static int button_probe(struct platform_device *pdev)
{
        struct device_node *np = pdev->dev.of_node;
        enum of_gpio_flags flags;

        int err;
        dev_t dev;
        struct device *mdevice;

        /* 申请按键描述变量资源 */
        btn_desc = (struct button_desc *)kmalloc(sizeof(struct button_desc), GFP_KERNEL);
        if (btn_desc == NULL)
        {
                goto error;
        }

        /* 申请按键 Gpio 资源 */
        btn_gpio = of_get_named_gpio_flags(np, "gpio", 0, &flags);
        if (!gpio_is_valid(btn_gpio))
        {
                goto descfree;
        }

        err = gpio_request(btn_gpio, "bind-gpio");
        if (err < 0)
        {
                printk("ledctrl-gpio%d request error\n", btn_gpio);
                goto gpiofree;
        }

        gpio_direction_input(btn_gpio);

        /* 注册中断资源 */
        btn_irq = gpio_to_irq(btn_gpio);
        err = request_irq(btn_irq, button_irq_handler, IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING | IRQF_SHARED, "bind-button", btn_desc);
        if (err != 0)
        {
                printk("request_irq failed. ret: %d  irq: %d\n", err, btn_irq);
                goto gpiofree;
        }

        /* 初始化定时器相关参数 */
        // init_timer(&button_timer);
        button_timer.function = button_timer_handler;
        add_timer(&button_timer);
        /* 注册字符设备 */
        err = alloc_chrdev_region(&dev, 0, 1, "button");
        if (err < 0)
        {
                printk(KERN_ALERT"Failed to alloc char dev region.\n");
                goto irqfree;
        }

        major = MAJOR(dev);
        minor = MINOR(dev);

        mcdev = cdev_alloc();
        if (!mcdev)
        {
        goto unregister;
    }

        cdev_init(mcdev, &button_fops);
        err = cdev_add(mcdev, MKDEV(major, minor), 1);
        if (err)
        {
                printk(KERN_INFO "cdev_add error\n");
                goto destroy_cdev;
        }

        mclass = class_create(THIS_MODULE, "button");
        if (IS_ERR(mclass))
        {
                err = PTR_ERR(mclass);
                printk(KERN_ALERT"Failed to create hello class.\n");
        goto destroy_cdev;
        }

        mdevice = device_create(mclass, NULL, MKDEV(major, minor), "%s", "button");
        if (err < 0)
        {
                printk(KERN_ALERT"Failed to create attribute val.\n");
                goto destroy_class;
        }

        return 0;

destroy_class:
        class_destroy(mclass);
        mclass = NULL;

destroy_cdev:
        cdev_del(mcdev);
        mcdev = NULL;

unregister:
        unregister_chrdev_region(MKDEV(major, minor), 1);

irqfree:
        free_irq(btn_irq, btn_desc);
        btn_irq = -1;
        del_timer(&button_timer);

gpiofree:
        gpio_free(btn_gpio);

descfree:
        kfree(btn_desc);
        btn_desc = NULL;

error:
        return -1;
}

static int button_remove(struct platform_device *pdev)
{
        if (btn_irq != -1)
                free_irq(btn_irq, btn_desc);

        if (gpio_is_valid(btn_gpio))
                gpio_free(btn_gpio);

        del_timer(&button_timer);

        if (mclass) {
                device_destroy(mclass, MKDEV(major, minor));
                class_destroy(mclass);
                mclass = NULL;
        }

        if (mcdev) {
                unregister_chrdev_region(MKDEV(major, minor), 1);
                cdev_del(mcdev);
                mcdev = NULL;
        }

        if (btn_desc) {
                kfree(btn_desc);
                btn_desc = NULL;
        }

        return 0;
}

static const struct of_device_id button_of_match[] = {
    { .compatible = "button" },
    { }
};

static struct platform_driver button_driver = {
    .probe      = button_probe,
        .remove = button_remove,
        .driver = {
                .name                   = "button-driver",
                .of_match_table = of_match_ptr(button_of_match),
        },
};

module_platform_driver(button_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("HouKaisen");




