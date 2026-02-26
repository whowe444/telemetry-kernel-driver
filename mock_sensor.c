/*
 * Simple Mock Sensor Driver with sysfs_notify and ioctl
 * Exposes temperature and humidity values via sysfs
 * Updates values every second with Gaussian noise
 * Notifies userspace of changes via sysfs_notify()
 * Supports ioctl to change temperature units (C/F)
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/random.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Will Howe");
MODULE_DESCRIPTION("Dynamic mock sensor driver with ioctl");
MODULE_VERSION("1.0");

/* ioctl commands */
#define SENSOR_IOC_MAGIC 'S'
#define SENSOR_IOC_SET_CELSIUS    _IO(SENSOR_IOC_MAGIC, 0)
#define SENSOR_IOC_SET_FAHRENHEIT _IO(SENSOR_IOC_MAGIC, 1)
#define SENSOR_IOC_GET_UNIT       _IOR(SENSOR_IOC_MAGIC, 2, int)

#define UNIT_CELSIUS    0
#define UNIT_FAHRENHEIT 1

/* Mock sensor values (always stored in Celsius internally) */
static int temperature_c = 25;
static int humidity = 60;

/* Base values for sensors */
static int temp_base = 25;
static int humidity_base = 60;

/* Temperature unit setting */
static int temp_unit = UNIT_CELSIUS;

/* Timer for periodic updates */
static struct timer_list sensor_timer;

/* Kobject pointer (needed for sysfs_notify) */
static struct kobject *sensor_kobj;

/* Character device for ioctl */
static dev_t dev_num;
static struct cdev sensor_cdev;
static struct class *sensor_class;
static struct device *sensor_device;

/* Convert Celsius to Fahrenheit */
static int celsius_to_fahrenheit(int celsius)
{
    return (celsius * 9 / 5) + 32;
}

/* Convert Fahrenheit to Celsius */
static int fahrenheit_to_celsius(int fahrenheit)
{
    return (fahrenheit - 32) * 5 / 9;
}

/* Get temperature in current unit */
static int get_temperature_in_unit(void)
{
    if (temp_unit == UNIT_FAHRENHEIT)
        return celsius_to_fahrenheit(temperature_c);
    return temperature_c;
}

/* Simple Box-Muller transform for Gaussian noise */
static int get_gaussian_noise(int mean, int stddev)
{
    unsigned int rand1, rand2;
    int noise;
    
    get_random_bytes(&rand1, sizeof(rand1));
    get_random_bytes(&rand2, sizeof(rand2));
    
    /* Simplified noise generation (not true Gaussian but close enough) */
    noise = (int)((rand1 % (2 * stddev + 1)) - stddev);
    
    return mean + noise;
}

/* Timer callback function - updates sensor values */
static void update_sensors(struct timer_list *t)
{
    /* Update temperature with stddev of 2 (in Celsius) */
    temperature_c = get_gaussian_noise(temp_base, 2);
    
    /* Update humidity with stddev of 5 */
    humidity = get_gaussian_noise(humidity_base, 5);
    
    /* Clamp values to reasonable ranges */
    if (temperature_c < 0) temperature_c = 0;
    if (temperature_c > 50) temperature_c = 50;
    if (humidity < 0) humidity = 0;
    if (humidity > 100) humidity = 100;
    
    /* Notify userspace that values have changed */
    sysfs_notify(sensor_kobj, NULL, "temperature");
    sysfs_notify(sensor_kobj, NULL, "humidity");
    
    /* Reschedule timer for 1 second later */
    mod_timer(&sensor_timer, jiffies + HZ);
}

/* Show function for temperature */
static ssize_t temperature_show(struct kobject *kobj, 
                                struct kobj_attribute *attr, 
                                char *buf)
{
    int temp = get_temperature_in_unit();
    const char *unit = (temp_unit == UNIT_FAHRENHEIT) ? "F" : "C";
    return sprintf(buf, "%d %s\n", temp, unit);
}

/* Store function for temperature (updates base value) */
static ssize_t temperature_store(struct kobject *kobj, 
                                 struct kobj_attribute *attr,
                                 const char *buf, size_t count)
{
    int value;
    sscanf(buf, "%d", &value);
    
    /* Convert to Celsius if needed */
    if (temp_unit == UNIT_FAHRENHEIT)
        temp_base = fahrenheit_to_celsius(value);
    else
        temp_base = value;
    
    sysfs_notify(sensor_kobj, NULL, "temperature");
    return count;
}

/* Show function for humidity */
static ssize_t humidity_show(struct kobject *kobj, 
                             struct kobj_attribute *attr, 
                             char *buf)
{
    return sprintf(buf, "%d\n", humidity);
}

/* Store function for humidity (updates base value) */
static ssize_t humidity_store(struct kobject *kobj, 
                              struct kobj_attribute *attr,
                              const char *buf, size_t count)
{
    sscanf(buf, "%d", &humidity_base);
    sysfs_notify(sensor_kobj, NULL, "humidity");
    return count;
}

/* Show function for unit */
static ssize_t unit_show(struct kobject *kobj,
                        struct kobj_attribute *attr,
                        char *buf)
{
    return sprintf(buf, "%s\n", 
                   (temp_unit == UNIT_FAHRENHEIT) ? "fahrenheit" : "celsius");
}

/* Define sysfs attributes */
static struct kobj_attribute temperature_attribute = 
    __ATTR(temperature, 0664, temperature_show, temperature_store);

static struct kobj_attribute humidity_attribute = 
    __ATTR(humidity, 0664, humidity_show, humidity_store);

static struct kobj_attribute unit_attribute =
    __ATTR(unit, 0444, unit_show, NULL);

/* Group attributes together */
static struct attribute *attrs[] = {
    &temperature_attribute.attr,
    &humidity_attribute.attr,
    &unit_attribute.attr,
    NULL,
};

static struct attribute_group attr_group = {
    .attrs = attrs,
};

/* Character device ioctl handler */
static long sensor_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    int unit;
    
    switch (cmd) {
    case SENSOR_IOC_SET_CELSIUS:
        printk(KERN_INFO "Mock Sensor: Unit set to Celsius\n");
        temp_unit = UNIT_CELSIUS;
        sysfs_notify(sensor_kobj, NULL, "temperature");
        sysfs_notify(sensor_kobj, NULL, "unit");
        break;
        
    case SENSOR_IOC_SET_FAHRENHEIT:
        printk(KERN_INFO "Mock Sensor: Unit set to Fahrenheit\n");
        temp_unit = UNIT_FAHRENHEIT;
        sysfs_notify(sensor_kobj, NULL, "temperature");
        sysfs_notify(sensor_kobj, NULL, "unit");
        break;
        
    case SENSOR_IOC_GET_UNIT:
        unit = temp_unit;
        if (copy_to_user((int __user *)arg, &unit, sizeof(int)))
            ret = -EFAULT;
        break;
        
    default:
        ret = -EINVAL;
    }
    
    return ret;
}

/* Character device file operations */
static struct file_operations sensor_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = sensor_ioctl,
};

/* Module initialization */
static int __init sensor_init(void)
{
    int ret;

    /* Create a kobject under /sys/kernel/ */
    sensor_kobj = kobject_create_and_add("mock_sensor", kernel_kobj);
    if (!sensor_kobj)
        return -ENOMEM;

    /* Create sysfs files */
    ret = sysfs_create_group(sensor_kobj, &attr_group);
    if (ret) {
        kobject_put(sensor_kobj);
        return ret;
    }

    /* Allocate character device number */
    ret = alloc_chrdev_region(&dev_num, 0, 1, "mock_sensor");
    if (ret < 0) {
        kobject_put(sensor_kobj);
        return ret;
    }

    /* Initialize character device */
    cdev_init(&sensor_cdev, &sensor_fops);
    sensor_cdev.owner = THIS_MODULE;

    /* Add character device */
    ret = cdev_add(&sensor_cdev, dev_num, 1);
    if (ret < 0) {
        unregister_chrdev_region(dev_num, 1);
        kobject_put(sensor_kobj);
        return ret;
    }

    /* Create device class */
    sensor_class = class_create("mock_sensor");
    if (IS_ERR(sensor_class)) {
        cdev_del(&sensor_cdev);
        unregister_chrdev_region(dev_num, 1);
        kobject_put(sensor_kobj);
        return PTR_ERR(sensor_class);
    }

    /* Create device file */
    sensor_device = device_create(sensor_class, NULL, dev_num, NULL, "mock_sensor");
    if (IS_ERR(sensor_device)) {
        class_destroy(sensor_class);
        cdev_del(&sensor_cdev);
        unregister_chrdev_region(dev_num, 1);
        kobject_put(sensor_kobj);
        return PTR_ERR(sensor_device);
    }

    /* Initialize and start the timer */
    timer_setup(&sensor_timer, update_sensors, 0);
    mod_timer(&sensor_timer, jiffies + HZ);

    printk(KERN_INFO "Mock Sensor Driver: Initialized with ioctl support\n");
    printk(KERN_INFO "Mock Sensor Driver: Device created at /dev/mock_sensor\n");
    return 0;
}

/* Module cleanup */
static void __exit sensor_exit(void)
{
    /* Stop the timer */
    del_timer_sync(&sensor_timer);
    
    /* Remove device and class */
    device_destroy(sensor_class, dev_num);
    class_destroy(sensor_class);
    cdev_del(&sensor_cdev);
    unregister_chrdev_region(dev_num, 1);
    
    /* Remove sysfs */
    kobject_put(sensor_kobj);
    
    printk(KERN_INFO "Mock Sensor Driver: Exiting\n");
}

module_init(sensor_init);
module_exit(sensor_exit);