/*
 * Simple Mock Sensor Driver
 * Exposes temperature and humidity values via sysfs
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/init.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Will Howe");
MODULE_DESCRIPTION("Simple mock sensor driver");
MODULE_VERSION("1.0");

/* Mock sensor values */
static int temperature = 25;
static int humidity = 60;

/* Show function for temperature */
static ssize_t temperature_show(struct kobject *kobj, 
                                struct kobj_attribute *attr, 
                                char *buf)
{
    return sprintf(buf, "%d\n", temperature);
}

/* Store function for temperature */
static ssize_t temperature_store(struct kobject *kobj, 
                                 struct kobj_attribute *attr,
                                 const char *buf, size_t count)
{
    sscanf(buf, "%d", &temperature);
    return count;
}

/* Show function for humidity */
static ssize_t humidity_show(struct kobject *kobj, 
                             struct kobj_attribute *attr, 
                             char *buf)
{
    return sprintf(buf, "%d\n", humidity);
}

/* Store function for humidity */
static ssize_t humidity_store(struct kobject *kobj, 
                              struct kobj_attribute *attr,
                              const char *buf, size_t count)
{
    sscanf(buf, "%d", &humidity);
    return count;
}

/* Define sysfs attributes */
static struct kobj_attribute temperature_attribute = 
    __ATTR(temperature, 0664, temperature_show, temperature_store);

static struct kobj_attribute humidity_attribute = 
    __ATTR(humidity, 0664, humidity_show, humidity_store);

/* Group attributes together */
static struct attribute *attrs[] = {
    &temperature_attribute.attr,
    &humidity_attribute.attr,
    NULL,
};

static struct attribute_group attr_group = {
    .attrs = attrs,
};

static struct kobject *sensor_kobj;

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
    if (ret)
        kobject_put(sensor_kobj);

    printk(KERN_INFO "Mock Sensor Driver: Initialized\n");
    return ret;
}

/* Module cleanup */
static void __exit sensor_exit(void)
{
    kobject_put(sensor_kobj);
    printk(KERN_INFO "Mock Sensor Driver: Exiting\n");
}

module_init(sensor_init);
module_exit(sensor_exit);