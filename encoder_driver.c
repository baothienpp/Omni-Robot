#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h> 
#include <linux/gpio.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>

static const int gpio_pin = 205; // GPIO_18
static int pulscount = 0;
static int irq_input_pin = -1;


static char command[256];
static char response[256];




static int            my_major_number;
static struct class*  my_device_class;
static struct device* my_device;

static irqreturn_t irq_handler( int irq, void *dev_id )
{
    
    pulscount++;
    return IRQ_HANDLED;
}


int my_read( struct file *filep, char *buffer, size_t count, loff_t *offp )
{
    int ret;

    sprintf(response,"%d",pulscount);

    ret = copy_to_user(buffer,response,strlen(response));

    if( ret != 0 )
        return -EINVAL;

    return count;
}

int my_write( struct file *filep, const char *buffer, size_t count, loff_t *offp )
{

    int ret;

    ret = copy_from_user( command, buffer, count );
    if( ret != 0 )
        return -EINVAL;

    // reset pulscount
    if(command[0] == '1'){
        pulscount = 0;
    }

    return count;
} 

static struct file_operations my_fops =
{
    .read = my_read,
    .write = my_write,
};


static int __init my_init(void)
{
    int rc;

    printk(KERN_INFO "Encoder Driver Initialized \n");

    //================================Character Device===========================
    my_major_number = register_chrdev(0, "encoder", &my_fops);
    if( my_major_number < 0 )
    {
        printk( KERN_ERR "register_chrdev failed, error %d\n", my_major_number );
        return  my_major_number;
    }

    my_device_class = class_create( THIS_MODULE, "encoderclass" );
    if( IS_ERR(my_device_class) )
    {
        printk( KERN_ERR "class_create failed, error %ld\n", PTR_ERR(my_device_class) );
        unregister_chrdev( my_major_number, "encoder" );
        return PTR_ERR(my_device_class);
    }  

    my_device = device_create( my_device_class, NULL, MKDEV(my_major_number, 0), NULL, "encoder" );
    if (IS_ERR(my_device))
    {
        printk( KERN_ERR "device_create failed, error %ld\n", PTR_ERR(my_device) );
        class_destroy( my_device_class );
        unregister_chrdev( my_major_number, "encoder" );
        return PTR_ERR(my_device);
    }

    //====================GPIO Interrupt=================================

    rc = gpio_request( gpio_pin, "motor_encoder" );
    if( rc < 0 ) {
        printk(KERN_ERR "my module: gpio_request failed with error %d\n", rc );
        return rc;
    }

    irq_input_pin = gpio_to_irq(gpio_pin);
    if( irq_input_pin < 0 ) {
        printk(KERN_ERR "my module: gpio_to_irq failed with error %d\n", rc );
        gpio_free(gpio_pin);
        return irq_input_pin;
    }

    // the string "my_gpio_handler" can be found in cat /proc/interrupts when module is loaded
    rc = request_irq( irq_input_pin, &irq_handler, IRQF_TRIGGER_RISING, "my_gpio_handler", NULL );
    if( rc < 0 ) {
        gpio_free(gpio_pin);
        printk(KERN_ERR "my module: request_irq failed with error %d\n", rc );
    }

    return 0;
}

static void __exit my_exit(void)
{
    device_destroy( my_device_class, MKDEV(my_major_number,0) );
    class_destroy( my_device_class );
    unregister_chrdev( my_major_number, "encoder" );
    //==============================
    printk(KERN_INFO "Encoder Driver Exit\n" );
    free_irq( irq_input_pin, NULL );
    gpio_free(gpio_pin);
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("F.B.");
MODULE_DESCRIPTION("Motor Encoder Driver");
