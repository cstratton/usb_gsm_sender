// Simple USBHost GPS Dongle for FRDM-KL46Z
#include "USBHost.h"
#include "decodeNMEA.h"

#define PL2303_SET_LINE_CODING 0x20

class USBHostGPS : public IUSBEnumerator {
public:

    /**
    * Constructor
    */
    USBHostGPS(int baud = 38400);

    /**
     * Try to connect a USB GPS device
     *
     * @return true if connection was successful
     */
    bool connect();

    /**
    * Check if a USB GPS is connected
    *
    * @returns true if a mouse is connected
    */
    bool connected();

    int readNMEA(char* data, int size, int timeout_ms) {
        host->bulkRead(dev, bulk_in, (uint8_t*)data, size);
        return bulk_in->getLengthTransferred();
    }
    void attachEventRaw(void (*ptr)(char* data, int size)) {
        if (ptr != NULL) {
            onUpdateRaw = ptr;
        }
    }

    decodeNMEA nmea;

protected:
    //From IUSBEnumerator
    virtual void setVidPid(uint16_t vid, uint16_t pid);
    virtual bool parseInterface(uint8_t intf_nb, uint8_t intf_class, uint8_t intf_subclass, uint8_t intf_protocol); //Must return true if the interface should be parsed
    virtual bool useEndpoint(uint8_t intf_nb, ENDPOINT_TYPE type, ENDPOINT_DIRECTION dir); //Must return true if the endpoint will be used

private:
    USBHost * host;
    USBDeviceConnected* dev;
    USBEndpoint* bulk_in;
    bool dev_connected;
    bool gps_device_found;
    int gps_intf;

    void rxHandler();
    void (*onUpdateRaw)(char* data, int size);
    uint8_t bulk_buf[64];
    int baud;
    void init();
};
