#include <stdio.h>
#include <stdlib.h>
#include <libusb-1.0/libusb.h>

#define STRING_DESCRIPTOR_SIZE 40

// ADB Interface Specifications
#define ADB_CLASS 0xff
#define ADB_SUBCLASS 0x42
#define ADB_PROTOCOL 0x1

static void print_open_error(int error)
{
  char *error_str;
  switch(error) {
  case LIBUSB_ERROR_NO_MEM:
	error_str = "No memory";
	break;
  case LIBUSB_ERROR_ACCESS:
	error_str = "Insufficient permissions";
	break;
  case LIBUSB_ERROR_NO_DEVICE:
	error_str = "Device has been disconnected";
	break;
  default:
	error_str = "Unknown error";
	break;
  }
  printf("%s", error_str);
}

static void access_device_handle(libusb_device *device, libusb_device_handle *handle) {
  struct libusb_device_descriptor *desc = malloc(sizeof(struct libusb_device_descriptor));
  int err = libusb_get_device_descriptor(device, desc);

  if (err != 0) {
	printf("Error reading descriptor (");
	print_open_error(err);
	printf(")");
  } else {
	unsigned char string_des[STRING_DESCRIPTOR_SIZE];

	int len = libusb_get_string_descriptor_ascii(handle, desc->iProduct, string_des, STRING_DESCRIPTOR_SIZE);
	if (len >= 0) {
	  printf("%s", string_des);
	} else {
	  printf("Error reading descriptor (");
	  print_open_error(len);
	  printf(")");
	}
  }
}

static int is_adb_interface(int usb_class, int usb_subclass, int usb_protocol) {
  //TODO: Check if vendor id of device is whitelisted
  return usb_class == ADB_CLASS && usb_subclass == ADB_SUBCLASS && usb_protocol == ADB_PROTOCOL;
}

static int check_adb(libusb_device *device, libusb_device_handle *handle) {
  struct libusb_device_descriptor *desc = malloc(sizeof(struct libusb_device_descriptor));
  int err = libusb_get_device_descriptor(device, desc);
  if (err != 0) {
	return 0;
  }
  
  struct libusb_config_descriptor *config;
  err = libusb_get_active_config_descriptor(device, &config);
  if (err != 0) {
	return 0;
  }

  const struct libusb_interface *interfaces = config->interface;
  /* TODO: Consider alternate interface settings
	 const struct libusb_interface_descriptor *interfaces = config->interface->altsetting;
	 int num_altsetting = config->interface->num_altsetting; */
  for (int i = 0; i < config->bNumInterfaces; i++) {
	const struct libusb_interface_descriptor *idesc = (interfaces + i)->altsetting;
	if (is_adb_interface(idesc->bInterfaceClass, idesc->bInterfaceSubClass, idesc->bInterfaceProtocol)) {
	  printf(" **ADB-compliant");
	}
  }

  libusb_free_config_descriptor(config);
  return 1;
}

int main(int argc, char **argv)
{ // When in rome... :(
  libusb_init(NULL);
  libusb_device **devices;
  ssize_t dev_cnt = libusb_get_device_list(NULL, &devices);
  if (dev_cnt < 0) {
	exit(1);
  }
  printf("Number of devices: %zd\n", dev_cnt);

  ssize_t i;

  for (i = 0; i < dev_cnt; i++) {
	libusb_device_handle *handle;
	int err = libusb_open(devices[i], &handle);
	printf("%zd: ", i);
	if (err == 0) {
	  access_device_handle(devices[i], handle);
	  check_adb(devices[i], handle);
	  printf("\n");
	  libusb_close(handle);
	} else {
	  printf("Error opening device (");
	  print_open_error(err);
	  printf(")\n");
	}
  }

  libusb_free_device_list(devices, 1);
  libusb_exit(NULL);
  return 0;
}
