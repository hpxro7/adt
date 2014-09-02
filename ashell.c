#include <stdio.h>
#include <stdlib.h>
#include <libusb-1.0/libusb.h>

#define STRING_DESCRIPTOR_SIZE 40

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
	printf(")\n");
  } else {
	unsigned char data[STRING_DESCRIPTOR_SIZE];

	int len = libusb_get_string_descriptor_ascii(handle, desc->iProduct, data, STRING_DESCRIPTOR_SIZE);
	if (len >= 0) {
	  //printf("bytes read: %d\n", len);
	  printf("%s\n", data);
	} else {
	  printf("Error reading descriptor (");
	  print_open_error(len);
	  printf(")\n");
	}
  }
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
	  libusb_close(handle);
	} else {
	  printf("Error reading device");
	  print_open_error(err);
	  printf("\n");
	}
  }

  libusb_free_device_list(devices, 1);
  libusb_exit(NULL);
  return 0;
}
