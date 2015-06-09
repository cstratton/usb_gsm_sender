#include "USBHostRSSI.h"

USBHostRSSI::USBHostRSSI()
{
    host = USBHost::getHostInst();
    init();
}

void USBHostRSSI::init()
{
    dev = NULL;
    int_in = NULL;
    onUpdate = NULL;
    dev_connected = false;
    bluetooth_device_found = false;
    bluetooth_intf = -1;
    seq = 0;
    
}

bool USBHostRSSI::connected() {
    return dev_connected;
}

bool USBHostRSSI::connect() {

    if (dev_connected) {
        return true;
    }
    
    for (uint8_t i = 0; i < MAX_DEVICE_CONNECTED; i++) {
        if ((dev = host->getDevice(i)) != NULL) {
            if(host->enumerate(dev, this)) {
                break;
            }
            if (bluetooth_device_found) {
                int_in = dev->getEndpoint(bluetooth_intf, INTERRUPT_ENDPOINT, IN);
                USB_DBG("int_in=%p", int_in);
                if (!int_in) {
                    break;
                }
                USB_INFO("New Bluetooth device: VID:%04x PID:%04x [dev: %p - intf: %d]", dev->getVid(), dev->getPid(), dev, bluetooth_intf);
                int_in->attach(this, &USBHostRSSI::rxHandler);
                int rc = host->interruptRead(dev, int_in, int_buf, sizeof(int_buf), false);
                USB_TEST_ASSERT(rc != USB_TYPE_ERROR);
                rc = cmdSend(HCI_OP_RESET);
                USB_TEST_ASSERT(rc == USB_TYPE_OK);
                dev_connected = true;
                return true;
            }
        }
    }
    init();
    return false;
}

void USBHostRSSI::rxHandler() {
    event(int_buf, int_in->getLengthTransferred());
    if (dev) {
        host->interruptRead(dev, int_in, int_buf, sizeof(int_buf), false);
    }
}

/*virtual*/ void USBHostRSSI::setVidPid(uint16_t vid, uint16_t pid)
{
    USB_DBG("vid:%04x pid:%04x", vid, pid);
    // we don't check VID/PID for mouse driver
}

/*virtual*/ bool USBHostRSSI::parseInterface(uint8_t intf_nb, uint8_t intf_class, uint8_t intf_subclass, uint8_t intf_protocol) //Must return true if the interface should be parsed
{
    USB_DBG("intf: %d class: %02x %02x %02x", intf_nb, intf_class, intf_subclass, intf_protocol);
    if (bluetooth_intf == -1 && intf_class == 0xe0) {
        bluetooth_intf = intf_nb;
        return true;
    }
    return false;
}

/*virtual*/ bool USBHostRSSI::useEndpoint(uint8_t intf_nb, ENDPOINT_TYPE type, ENDPOINT_DIRECTION dir) //Must return true if the endpoint will be used
{
    USB_DBG("intf_nb=%d type=%d dir=%d", intf_nb, type, dir);

    if (intf_nb == bluetooth_intf) {
        if (type == INTERRUPT_ENDPOINT && dir == IN) {
            bluetooth_device_found = true;
            return true;
        }
    }
    return false;
}

void USBHostRSSI::event(uint8_t* data, int len) {
    CTASSERT(sizeof(BD_ADDR) == 6);
    CTASSERT(sizeof(inquiry_with_rssi_info) == 14);
    inquiry_with_rssi_info* info;
    int max_period_length = 25;
    int min_period_length = 20;
    int inquiry_length = 15;
    USB_TYPE rc;
    if (len > 0) {
        USB_DBG_HEX(data, len);
        hci_event* event = reinterpret_cast<hci_event*>(data);
        switch(event->evt) {
            case HCI_EV_CMD_COMPLETE:
                switch(event->c.op) {
                    case HCI_OP_RESET:
                        wait_ms(500);
                        rc = cmdSend(HCI_OP_WRITE_INQUIRY_MODE, "B", 0x01); // with RSSI
                        USB_TEST_ASSERT(rc == USB_TYPE_OK);
                        if (rc != USB_TYPE_OK) {
                        }
                        break;
                    case HCI_OP_WRITE_INQUIRY_MODE:
                        rc = cmdSend(HCI_OP_PERIODIC_INQUIRY, "HHBBBBB", 
                                            max_period_length, min_period_length, 0x33, 0x8B, 0x9E, inquiry_length, 0);
                        USB_TEST_ASSERT(rc == USB_TYPE_OK);
                        break;
                    default:
                        break;
                }
                break;
            case HCI_EV_INQUIRY_RESULT_WITH_RSSI:
                //USB_DBG_HEX(buf, r);
                info = reinterpret_cast<inquiry_with_rssi_info*>(event->c.data);
                if (onUpdate) {
                    (*onUpdate)(info);
                }
                break;
            default:
                break;
        }        
    }
}

USB_TYPE USBHostRSSI::cmdSend(uint16_t op)
{
   return cmdSendSub(op, NULL, 0);
}

USB_TYPE USBHostRSSI::cmdSend(uint16_t op, const char* fmt, ...) 
{   
    va_list vl;
    va_start(vl, fmt);
    uint8_t buf[255];
    int pos = 0;
    char* name;
    int name_len;
    uint16_t h;
    BD_ADDR* bdaddr;
    for(int i = 0; fmt[i]; i++) {
        switch(fmt[i]) {
            case 's':
                name = va_arg(vl, char*);
                name_len = strlen(name)+1;
                memcpy(buf+pos, name, name_len);
                pos += name_len;
                break;
            case 'B':
                buf[pos++] = va_arg(vl, int);
                break;
            case 'H':
                h = va_arg(vl, int);
                buf[pos++] = h;
                buf[pos++] = h>>8;
                break;
            case 'A':
                bdaddr = va_arg(vl, BD_ADDR*);
                memcpy(buf+pos, bdaddr, 6);
                pos += 6;
                break;
            default:
                USB_DBG("op=%04X fmt=%s i=%d", op, fmt, i);
                break;
        }
    }
    return cmdSendSub(op, buf, pos);
}

USB_TYPE USBHostRSSI::cmdSendSub(uint16_t op, const uint8_t* data, int size)
{
    uint8_t* buf = new uint8_t[size+3];
    buf[0] = op;
    buf[1] = op>>8;
    buf[2] = size;
    if (data) {
        memcpy(buf+3, data, size);
    }
    USB_TYPE rc = host->controlWrite(dev, USB_REQUEST_TYPE_CLASS, 0, 0, 0, buf, size+3);
    USB_TEST_ASSERT(rc == USB_TYPE_OK);
    delete[] buf;
    return rc;
}
