#include "USBHostE303.h" //covers both pre- and post-

USBHostE303PreSwitch::USBHostE303PreSwitch() {
    host = USBHost::getHostInst();
    init();
}


void USBHostE303PreSwitch::init() {
  printf("::init!!!!!!!!!!!!!1\n");
    dev = NULL;
    report_id = 0;
    onKey = NULL;
    onKeyCode = NULL;
    dev_connected = false;
    keyboard_intf = -1;
    preswitch_device_found = false;
    bulk_out = NULL;
    bulk_in = NULL;
}

bool USBHostE303PreSwitch::connected() {
    return dev_connected;
}


int USBHostE303PreSwitch::doswitch() {
 unsigned char cmd[] = {
      0x55, 0x53, 0x42, 0x43, 0x12, 0x34, 0x56, 0x78, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 
      0x06, 0x20, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  int res = host->bulkWrite(dev, bulk_out,(uint8_t *)cmd, 31);
  printf("writing 31 byte command to endpoint %x returns %d\n", bulk_out, res);
  return res;
}

int USBHostE303PreSwitch::bulk_write(void *data, int len) {
  printf("trying to write to interface %d\n", keyboard_intf);
  int rv = host->bulkWrite(dev, bulk_out, (uint8_t *)data, len);
  return (checkResult(rv, bulk_out)) ? -1 : 0;
}

int USBHostE303PreSwitch::bulk_read(void *data, int len) {
  int rv = host->bulkRead(dev, bulk_in, (uint8_t *)data, len);
  return (checkResult(rv, bulk_in)) ? -1 : 0;
}

int USBHostE303PreSwitch::checkResult(uint8_t res, USBEndpoint * ep) {
    // if ep stalled: send clear feature
    if (res == USB_TYPE_STALL_ERROR) {
        res = host->controlWrite(   dev,
                                    USB_RECIPIENT_ENDPOINT | USB_HOST_TO_DEVICE | USB_REQUEST_TYPE_STANDARD
,
                                    CLEAR_FEATURE,
                                    0, ep->getAddress(), NULL, 0);
        // set state to IDLE if clear feature successful
        if (res == USB_TYPE_OK) {
            ep->setState(USB_TYPE_IDLE);
        }
    }

    if (res != USB_TYPE_OK)
        return -1;

    return 0;
}

void do_reset() {
#define RESTART_ADDR       0xE000ED0C
#define USB0_USBTR0_ADDR 0x4007208C
  #define READ_RESTART()     (*(volatile uint32_t *)RESTART_ADDR)
  #define WRITE_RESTART(val) ((*(volatile uint32_t *)RESTART_ADDR) = (val))
  #define WRITE_USBTR0(val) ((*(volatile uint8_t *)USB0_USBTR0_ADDR) = (val))
  WRITE_USBTR0(0x80); //reset USB
  wait_ms(10);
  WRITE_RESTART(0x5FA0004);
}

bool USBHostE303PreSwitch::connect() {
  printf("printf test\n");
  puts("puts test\n");
    if (dev_connected) {
        return true;
    }
    
    for (uint8_t i = 0; i < MAX_DEVICE_CONNECTED; i++) {
        if ((dev = host->getDevice(i)) != NULL) {

            if (host->enumerate(dev, this))
                break;
            
            if (preswitch_device_found) {
	      //    int_in = dev->getEndpoint(keyboard_intf, INTERRUPT_ENDPOINT, IN);
	      printf("trying to get endpoint\r\n");
	      bulk_out = dev->getEndpoint(keyboard_intf, BULK_ENDPOINT, OUT);
	      bulk_in = dev->getEndpoint(keyboard_intf, BULK_ENDPOINT, IN);
	      printf("got endpoints\r\n");
              //  if (!int_in)
              //      break;
                
                USB_INFO("New E303PreSwitch device: VID:%04x PID:%04x [dev: %p - intf: %d]", dev->getVid(), dev->getPid(), dev, keyboard_intf);
                dev->setName("E303PreSwitch", keyboard_intf);
                host->registerDriver(dev, keyboard_intf, this, &USBHostE303PreSwitch::init);
                bulk_in->attach(this, &USBHostE303PreSwitch::rxHandler);
		//                int_in->attach(this, &USBHostE303PreSwitch::rxHandler);
                //host->interruptRead(dev, int_in, report, int_in->getSize(), false);
                
                dev_connected = true;
		printf("pre-switch found, going for switch\n");
		doswitch();
		printf("going for post-switch\n");
		printf("ah, heck with that, let's reboot!\n");
		wait_ms(1000);
		do_reset();
		while (!connect())
		  printf("trying again\n"); //now go for the post-switch
		printf("post switch found, ready to roll\n");
		
                //return true;
            }

            if (postswitch_device_found) {
	      printf("trying to get endpoint\r\n");
	      bulk_out = dev->getEndpoint(keyboard_intf, BULK_ENDPOINT, OUT);
	      bulk_in = dev->getEndpoint(keyboard_intf, BULK_ENDPOINT, IN);
	      printf("got endpoints\r\n");
                
	      USB_INFO("New E303PostSwitch device: VID:%04x PID:%04x [dev: %p - intf: %d]", dev->getVid(), dev->getPid(), dev, keyboard_intf);
                dev->setName("E303PostSwitch", keyboard_intf);
                host->registerDriver(dev, keyboard_intf, this, &USBHostE303PreSwitch::init);
		//  bulk_in->attach(this, &USBHostE303PreSwitch::rxHandler);
		//                int_in->attach(this, &USBHostE303PreSwitch::rxHandler);
                //host->interruptRead(dev, int_in, report, int_in->getSize(), false);
                
                dev_connected = true;
		printf("all set\n");
		return true;
            }
        }
    }
    init();
    return false;
}

void USBHostE303PreSwitch::rxHandler() {
    int len = bulk_in->getLengthTransferred();
    printf("input length %d\n", len);
    int index = (len == 9) ? 1 : 0;
    int len_listen = bulk_in->getSize();
    uint8_t key = 0;
    // if (len == 8 || len == 9) {
    //     uint8_t modifier = (report[index] == 4) ? 3 : report[index];
    //     len_listen = len;
    //     key = keymap[modifier][report[index + 2]];
    //     if (key && onKey) {
    //         (*onKey)(key);
    //     }
    //     if ((report[index + 2] || modifier) && onKeyCode) {
    //         (*onKeyCode)(report[index + 2], modifier);
    //     }
    // }
    if (dev && bulk_in)
        host->bulkRead(dev, bulk_in, report, len_listen, false);
}

/*virtual*/ void USBHostE303PreSwitch::setVidPid(uint16_t vid, uint16_t pid)
{
  printf("contemplating vid %04x pid %04x\n", vid, pid);
   // we don't check VID/PID for keyboard driver
  if ((vid == 0x12d1) && (pid == 0x14fe)) {
    printf("Claiming as a preswitch device!\n");
    preswitch_device_found = 1;
    postswitch_device_found = 0;
  }

  if ((vid == 0x12d1) && (pid == 0x1506)) {
    printf("Claiming as a postswitch device! YEAH NOW WERE COOKING WITH GAS\n");
    preswitch_device_found = 0;
    postswitch_device_found = 1;
  }
}

/*virtual*/ bool USBHostE303PreSwitch::parseInterface(uint8_t intf_nb, uint8_t intf_class, uint8_t intf_subclass, uint8_t intf_protocol) //Must return true if the interface should be parsed
{
  printf("contemplating interface %02x class %02x sub %02x\n", intf_nb, intf_class, intf_subclass);
  if (preswitch_device_found && (intf_nb == 1)) keyboard_intf = intf_nb;
  if (postswitch_device_found && (intf_nb == 0)) keyboard_intf = intf_nb;
  return (keyboard_intf == intf_nb);
   if ((keyboard_intf == -1) && 
        (intf_class == HID_CLASS) &&
        (intf_subclass == 0x01) &&
        (intf_protocol == 0x01)) {
        keyboard_intf = intf_nb;
        return true;
    }
    return false;
}

/*virtual*/ bool USBHostE303PreSwitch::useEndpoint(uint8_t intf_nb, ENDPOINT_TYPE type, ENDPOINT_DIRECTION dir) //Must return true if the endpoint will be used
{

  int use = (/*(type == 2) && */(intf_nb == keyboard_intf));
  printf("contemplating endpoint interface %02x type %02x dir %02x : %s\n", intf_nb, type, dir, use ? "use" : "decline");
  //this worked, but not what I want keyboard_intf = intf_nb;

  return use;
    if (intf_nb == keyboard_intf) {
        if (type == INTERRUPT_ENDPOINT && dir == IN) {
            preswitch_device_found = true;
            return true;
        }
    }
    return false;
}

/*
uint8_t UHS_USB_HOST_BASE::EPClearHalt(uint8_t addr, uint8_t ep) {
        return ctrlReq(addr, USB_SETUP_HOST_TO_DEVICE | USB_SETUP_TYPE_STANDARD | USB_SETUP_RECIPIENT_ENDPO
INT, USB_REQUEST_CLEAR_FEATURE, USB_FEATURE_ENDPOINT_HALT, 0, ep, 0, 0, NULL);
}
*/
