#include <stdio.h>
#include <stdlib.h>
#include <libusb-1.0/libusb.h>

#define STRING_DESCRIPTOR_SIZE 40
#define ADB_DEVICE_BUFFER_SIZE 3

// ADB Interface Specifications
#define ADB_CLASS 0xff
#define ADB_SUBCLASS 0x42
#define ADB_PROTOCOL 0x1

static void print_open_error(int error)
{
  const char *error_str;
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

static void print_device_info(libusb_device *device, libusb_device_handle *handle) {
  struct libusb_device_descriptor desc;
  int err = libusb_get_device_descriptor(device, &desc);

  if (err != 0) {
	printf("Error reading descriptor (");
	print_open_error(err);
	printf(")");
  } else {
	unsigned char string_des[STRING_DESCRIPTOR_SIZE];

	int len = libusb_get_string_descriptor_ascii(handle, desc.iProduct, string_des, STRING_DESCRIPTOR_SIZE);
	if (len >= 0) {
	  printf("%s", string_des);
	} else {
	  printf("Error reading descriptor (");
	  print_open_error(len);
	  printf(")");
	}
  }
}

static void print_devices(libusb_device **devices, ssize_t count) {
  for (ssize_t i = 0; i < count; i++) {
	libusb_device_handle *handle;
	int err = libusb_open(devices[i], &handle);
	printf("%zd) ", i + 1);
	if (err == 0) {
	  print_device_info(devices[i], handle);
	  printf("\n");
	  libusb_close(handle);
	} else {
	  printf("Error opening device (");
	  print_open_error(err);
	  printf(")\n");
	}
  }
}

static int is_adb_interface(int usb_class, int usb_subclass, int usb_protocol) {
  //TODO: Check if vendor id of device is whitelisted
  return usb_class == ADB_CLASS && usb_subclass == ADB_SUBCLASS && usb_protocol == ADB_PROTOCOL;
}

static int has_adb_endpoints(libusb_device *device, libusb_device_handle *handle) {
  struct libusb_device_descriptor desc;
  int err = libusb_get_device_descriptor(device, &desc);
  if (err != 0) {
	return 0;
  }

  struct libusb_config_descriptor *config;
  err = libusb_get_active_config_descriptor(device, &config);
  if (err) {
	return 0;
  }

  const struct libusb_interface *interfaces = config->interface;
  int num_interfaces = config->bNumInterfaces;
  // TODO: Consider alternate interface settings
  // const struct libusb_interface_descriptor *interfaces = config->interface->altsetting;
  // int num_altsetting = config->interface->num_altsetting;
  for (int i = 0; i < num_interfaces; i++) {
	const struct libusb_interface_descriptor *idesc = (interfaces + i)->altsetting;
	if (is_adb_interface(idesc->bInterfaceClass, idesc->bInterfaceSubClass, idesc->bInterfaceProtocol)) {
	  return 1;
	}
  }

  libusb_free_config_descriptor(config);
  return 0;
}

// Filters all devices compliant with the ADB protocol from src to dst, returning the
// number of compliant devices found. The number of found devices is bound by max. Max
// must be greater than or equal to the number of src devices.
static int filter_adb_devices(libusb_device **src, libusb_device **dst, int max) {
  int num_found = 0;
  for (int i = 0; i < max; i++) {
	libusb_device_handle *handle;
	int err = libusb_open(src[i], &handle);
	if (err) {
	  continue;
	}
	if (has_adb_endpoints(src[i], handle)) {
	  dst[num_found++] = src[i];
	}
	libusb_close(handle);
  }
  return num_found;
}

static void adb_shell(libusb_device *device) {
  libusb_device_handle *handle;
  int err = libusb_open(device, &handle);
  if (err) {
	printf("Could not connect to device");
	exit(1);
  }
}

int main(int argc, char **argv)
{ // When in rome... :(
  libusb_init(NULL);
  libusb_device **devices;
  ssize_t cnt = libusb_get_device_list(NULL, &devices);
  if (cnt < 0) {
	fprintf(stderr, "Could not obtain device list\n")
	exit(1);
  }

  print_devices(devices, cnt);
  
  libusb_device **adb_devices = malloc(sizeof(libusb_device*) * cnt);
  int devices_found = filter_adb_devices(devices, adb_devices, cnt);
  if (!devices_found) {
	printf("No adb compatible devices found\n");
  } else {
	printf("Adb compatible devices:\n");
	print_devices(adb_devices, devices_found);
	adb_shell(adb_devices[0]);
  }
  free(adb_devices);

  libusb_free_device_list(devices, 1);
  libusb_exit(NULL);
  return 0;
}
