#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/input.h>
#include <linux/uinput.h>

// Module information
MODULE_DESCRIPTION("USB driver for Optical touch screen");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Optical touch screen");

// Driver and device information
#define DRIVER_NAME "OpticalTouchDevice"
#define DEVICE_NAME "Otd"

// USB device IDs for supported devices
static struct usb_device_id const dev_table[] = {
    { USB_DEVICE(0x2621, 0x2201) },
    { USB_DEVICE(0x2621, 0x4501) },
    {}
};
MODULE_DEVICE_TABLE(usb, dev_table);

// Input device structure for uinput
static struct input_dev *uinput_dev;

// Probe function called when a matching device is found
static int otd_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
    // Extract USB device information
    struct usb_device *udev = interface_to_usbdev(intf);
    struct usb_device_descriptor desc;
    struct usb_host_interface *interface;
    struct usb_endpoint_descriptor *endpoint;
    int i, endpoint_in = 0, endpoint_out = 0;

    // Read the device descriptor to get device information
    if (usb_get_descriptor(udev, USB_DT_DEVICE, 0, &desc, sizeof(desc))) {
        printk(KERN_ERR "Failed to get device descriptor\n");
        return -ENODEV;
    }

    // Find the bulk endpoints for communication
    interface = intf->cur_altsetting;
    for (i = 0; i < interface->desc.bNumEndpoints; i++) {
        endpoint = &interface->endpoint[i].desc;
        if (usb_endpoint_is_bulk_in(endpoint))
            endpoint_in = endpoint->bEndpointAddress;
        else if (usb_endpoint_is_bulk_out(endpoint))
            endpoint_out = endpoint->bEndpointAddress;
    }

    // Initialize uinput device
    uinput_dev = input_allocate_device();
    if (!uinput_dev) {
        printk(KERN_ERR "Failed to allocate input device\n");
        return -ENOMEM;
    }

    // Set input device properties
    __set_bit(EV_KEY, uinput_dev->evbit);
    __set_bit(BTN_TOUCH, uinput_dev->keybit);
    __set_bit(EV_ABS, uinput_dev->evbit);
    __set_bit(ABS_MT_SLOT, uinput_dev->absbit);
    __set_bit(ABS_MT_POSITION_X, uinput_dev->absbit);
    __set_bit(ABS_MT_POSITION_Y, uinput_dev->absbit);
    __set_bit(ABS_MT_TOUCH_MAJOR, uinput_dev->absbit);

    // Set multi-touch parameters (manually since input_mt_init_slots is not available)
    uinput_dev->absinfo[ABS_MT_SLOT].maximum = 1;
    uinput_dev->absinfo[ABS_MT_SLOT].resolution = 0;
    input_set_abs_params(uinput_dev, ABS_MT_SLOT, 0, 1, 0, 0);
    input_set_abs_params(uinput_dev, ABS_MT_POSITION_X, 0, 32767, 0, 0);
    input_set_abs_params(uinput_dev, ABS_MT_POSITION_Y, 0, 32767, 0, 0);
    input_set_abs_params(uinput_dev, ABS_MT_TOUCH_MAJOR, 0, 32767, 0, 0);

    // Register uinput device
    if (input_register_device(uinput_dev)) {
        printk(KERN_ERR "Failed to register input device\n");
        input_free_device(uinput_dev);
        return -ENOMEM;
    }

    // Set a pointer to the USB device in the interface data
    usb_set_intfdata(intf, udev);
    return 0;
}

// Disconnect function called when the device is removed
static void otd_disconnect(struct usb_interface *intf)
{
    // Get the USB device pointer from the interface data
    struct usb_device *udev = usb_get_intfdata(intf);

    // Unregister and free uinput device
    if (uinput_dev) {
        input_unregister_device(uinput_dev);
        input_free_device(uinput_dev);
    }

    // Clear the pointer to the USB device in the interface data
    usb_set_intfdata(intf, NULL);
}

// USB driver structure
static struct usb_driver otd_driver = {
    .name = DRIVER_NAME,
    .probe = otd_probe,
    .disconnect = otd_disconnect,
    .id_table = dev_table,
};

// Module initialization function
static int otd_init(void)
{
    int result;

    // Register the USB driver
    result = usb_register(&otd_driver);
    if (result) {
        printk(KERN_ERR "usb_register failed. Error number %d\n", result);
        return result;
    }

    return 0;
}

// Module exit function
static void otd_exit(void)
{
    // Deregister the USB driver
    usb_deregister(&otd_driver);
}

// Specify the initialization and exit functions
module_init(otd_init);
module_exit(otd_exit);

