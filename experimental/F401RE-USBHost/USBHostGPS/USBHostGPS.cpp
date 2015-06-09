#include "USBHostGPS.h"

USBHostGPS::USBHostGPS(int baud_)
{
    host = USBHost::getHostInst();
    init();
    baud = baud_;
}

void USBHostGPS::init() {
    dev = NULL;
    bulk_in = NULL;
    onUpdateRaw = NULL;
    dev_connected = false;
    gps_device_found = false;
    gps_intf = -1;
}

bool USBHostGPS::connected() {
    return dev_connected;
}

bool USBHostGPS::connect() {

    if (dev_connected) {
        return true;
    }
    
    for (uint8_t i = 0; i < MAX_DEVICE_CONNECTED; i++) {
        if ((dev = host->getDevice(i)) != NULL) {
            if(host->enumerate(dev, this)) {
                break;
            }
            if (gps_device_found) {
                bulk_in = dev->getEndpoint(gps_intf, BULK_ENDPOINT, IN);
                USB_TEST_ASSERT(bulk_in);
                // stop bit = 1, parity = none, 8bit
                uint8_t data[] = {baud&0xff, baud>>8, baud>>16, baud>>24, 0x00, 0x00, 0x08};
                USB_TYPE rc = host->controlWrite(dev, 0x21, PL2303_SET_LINE_CODING, 0, 0, data, sizeof(data));
                USB_TEST_ASSERT(rc == USB_TYPE_OK);
                USB_INFO("New GPS device: VID:%04x PID:%04x [dev: %p - intf: %d]", dev->getVid(), dev->getPid(), dev, gps_intf);
                bulk_in->attach(this, &USBHostGPS::rxHandler);
                host->bulkRead(dev, bulk_in, bulk_buf, bulk_in->getSize(), false);
                
                dev_connected = true;
                return true;
            }
        }
    }
    init();
    return false;
}

void USBHostGPS::rxHandler() {
    int len = bulk_in->getLengthTransferred();
    if (onUpdateRaw) {
        (*onUpdateRaw)((char*)bulk_buf, len);
    }
    nmea.inputNMEA((char*)bulk_buf, len);

    if (dev) {
        host->bulkRead(dev, bulk_in, bulk_buf, bulk_in->getSize(), false);
    }
}

/*virtual*/ void USBHostGPS::setVidPid(uint16_t vid, uint16_t pid)
{
    USB_DBG("vid:%04x pid:%04x", vid, pid);
    if (pid == 0x2303) {
        gps_device_found = true;
    }
}

/*virtual*/ bool USBHostGPS::parseInterface(uint8_t intf_nb, uint8_t intf_class, uint8_t intf_subclass, uint8_t intf_protocol) //Must return true if the interface should be parsed
{
    USB_DBG("intf: %d class: %02x %02x %02x", intf_nb, intf_class, intf_subclass, intf_protocol);
    if (gps_device_found) {
        if (gps_intf == -1) {
            gps_intf = intf_nb;
            return true;
        }
    }    
    return false;
}

/*virtual*/ bool USBHostGPS::useEndpoint(uint8_t intf_nb, ENDPOINT_TYPE type, ENDPOINT_DIRECTION dir) //Must return true if the endpoint will be used
{
    USB_DBG("intf_nb=%d type=%d dir=%d", intf_nb, type, dir);
    if (gps_device_found) {
        if (intf_nb == gps_intf) {
            if (type == BULK_ENDPOINT && dir == IN) {
                return true;
            }
        }
    }
    return false;
}

