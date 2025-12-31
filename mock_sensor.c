/*
 * Simple Mock Sensor Driver with sysfs_notify
 * Exposes temperature and humidity values via sysfs
 * Updates values every second with Gaussian noise
 * Notifies userspace of changes via sysfs_notify()
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/random.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Will Howe");
MODULE_DESCRIPTION("Dynamic mock sensor driver with notifications");
MODULE_VERSION("1.0");

/* Mock sensor values */
static int temperature = 25; // Air Temperature in Farenheit
static int humidity = 60; // Relative Humidity in %

/* Base values for sensors */
static int temp_base = 25; // Air Temperature in Farenheit
static int humidity_base = 60; // Relative Humidity in %

/* Timer for periodic updates */
static struct timer_list sensor_timer;

/* Kobject pointer (needed for sysfs_notify) */
static struct kobject *sensor_kobj;

/* Attribute pointers (needed for sysfs_notify) */
static struct kobj_attribute temperature_attribute;
static struct kobj_attribute humidity_attribute;

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
    /* Update temperature with stddev of 2 */
    temperature = get_gaussian_noise(temp_base, 2);
    
    /* Update humidity with stddev of 5 */
    humidity = get_gaussian_noise(humidity_base, 5);
    
    /* Clamp values to reasonable ranges */
    if (temperature < 0) temperature = 0;
    if (temperature > 50) temperature = 50;
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
    return sprintf(buf, "%d\n", temperature);
}

/* Store function for temperature (updates base value) */
static ssize_t temperature_store(struct kobject *kobj, 
                                 struct kobj_attribute *attr,
                                 const char *buf, size_t count)
{
    sscanf(buf, "%d", &temp_base);
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

    /* Initialize and start the timer */
    timer_setup(&sensor_timer, update_sensors, 0);
    mod_timer(&sensor_timer, jiffies + HZ);

    printk(KERN_INFO "Mock Sensor Driver: Initialized with sysfs_notify\n");
    return 0;
}

/* Module cleanup */
static void __exit sensor_exit(void)
{
    /* Stop the timer */
    del_timer_sync(&sensor_timer);
    
    kobject_put(sensor_kobj);
    printk(KERN_INFO "Mock Sensor Driver: Exiting\n");
}

module_init(sensor_init);
module_exit(sensor_exit);