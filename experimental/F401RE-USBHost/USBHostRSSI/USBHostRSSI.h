// Simple USBHost Bluetooth RSSI for FRDM-KL46Z
#pragma once
#include "USBHost.h"
#include <stdarg.h>

#define HCI_OP_INQUIRY               0x0401
#define HCI_OP_INQUIRY_CANCEL        0x0402
#define HCI_OP_PERIODIC_INQUIRY      0x0403
#define HCI_OP_EXIT_PERIODIC_INQUIRY 0x0404
#define HCI_OP_REMOTE_NAME_REQ       0x0419
#define HCI_OP_RESET                 0x0c03
#define HCI_OP_WRITE_LOCAL_NAME      0x0c13
#define HCI_OP_WRITE_SCAN_ENABLE     0x0c1a
#define HCI_OP_WRITE_CLASS_OF_DEV    0x0c24
#define HCI_OP_WRITE_INQUIRY_MODE    0x0c45
#define HCI_OP_READ_EXTENDED_INQUIRY_RESPONSE  0x0c51
#define HCI_OP_WRITE_EXTENDED_INQUIRY_RESPONSE 0x0c52
#define HCI_OP_READ_BD_ADDR          0x1009

#define HCI_EV_INQUIRY_COMPLETE         0x01
#define HCI_EV_INQUIRY_RESULT           0x02
#define HCI_EV_REMOTE_NAME              0x07
#define HCI_EV_CMD_COMPLETE             0x0e
#define HCI_EV_CMD_STATUS               0x0f
#define HCI_EV_INQUIRY_RESULT_WITH_RSSI 0x22
#define HCI_EV_EXTENDED_INQUIRY_RESULT  0x2f

#pragma pack(push,1)
struct BD_ADDR {
    uint8_t addr[6];
    void set(char* s) {
        char* p = s;
        for(int i = 5; i >= 0; i--) {
            addr[i] =  strtol(p, &p, 16);
            if (*p == ':') {
                p++;
            }
        }
    }
    void str(char* buf, size_t size) {
        snprintf(buf, size, "%02X:%02X:%02X:%02X:%02X:%02X", addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
    }
    void str_mask(char* buf, size_t size) {
        snprintf(buf, size, "%02X:%02X:%02X:xx:xx:xx", addr[5], addr[4], addr[3]);
    }
    bool eq(BD_ADDR* a) {
        return memcmp(addr, a->addr, 6) == 0;
    }
    bool eq_vendor(BD_ADDR* a) {
        return memcmp(addr+3, a->addr+3, 3) == 0;
    }
};

struct inquiry_info {
    BD_ADDR bdaddr;
    uint8_t page_scan_repetition_mode;
    uint8_t reserved[2];
    uint8_t dev_class[3];
    uint16_t clock_offset;
};

struct inquiry_with_rssi_info {       // offset
    BD_ADDR bdaddr;                   // +0
    uint8_t page_scan_repetition_mode;// +6
    uint8_t reserved[1];              // +7
    uint8_t class_of_device[3];       // +8
    uint16_t clock_offset;            // +11
    int8_t rssi;                      // +13
};                                    // +14

struct extended_inquiry_info {
    BD_ADDR bdaddr;
    uint8_t page_scan_repetition_mode;
    uint8_t reserved[1];
    uint8_t class_of_device[3];
    uint16_t clock_offset;
    int8_t rssi;
    uint8_t extended_inquiry_response[240];
};

struct hci_event {
    uint8_t evt;
    uint8_t len;
    uint8_t status;
    union {
        uint16_t op;
        uint8_t data[];
    } c;
};
#pragma pack(pop)

class USBHostRSSI : public IUSBEnumerator {
public:

    /**
    * Constructor
    */    
    USBHostRSSI();

    /**
     * Try to connect a BT device
     *
     * @return true if connection was successful
     */
    bool connect();

    /**
    * Check if a mouse is connected
    *
    * @returns true if a BT is connected
    */
    bool connected();

    void attachEvent(void (*ptr)(inquiry_with_rssi_info* info)) {
        if (ptr != NULL) {
            onUpdate = ptr;
        }
    }
    //Report* report;

protected:
    //From IUSBEnumerator
    virtual void setVidPid(uint16_t vid, uint16_t pid);
    virtual bool parseInterface(uint8_t intf_nb, uint8_t intf_class, uint8_t intf_subclass, uint8_t intf_protocol); //Must return true if the interface should be parsed
    virtual bool useEndpoint(uint8_t intf_nb, ENDPOINT_TYPE type, ENDPOINT_DIRECTION dir); //Must return true if the endpoint will be used

private:
    USBHost * host;
    USBDeviceConnected * dev;
    USBEndpoint * int_in;
    bool dev_connected;
    bool bluetooth_device_found;
    int bluetooth_intf;

    void rxHandler();
    void (*onUpdate)(inquiry_with_rssi_info* info);
    uint8_t int_buf[64];
    int seq;
    void event(uint8_t* data, int size);
    USB_TYPE cmdSend(uint16_t op);
    USB_TYPE cmdSend(uint16_t op, const char* fmt, ...);
    USB_TYPE cmdSendSub(uint16_t op, const uint8_t* data, int size);
    void init();
};

