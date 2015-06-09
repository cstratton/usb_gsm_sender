#pragma once
#include "mbed.h"
#include "USBHostTypes.h"
#include "USBEndpoint.h"

#define GET_CONFIGURATION 8

// TOK_PID[5:2]
#define Bus_Timeout 0x00
#define Data_Error 0x0f

enum ODD_EVEN {
    ODD = 0,
    EVEN = 1,
};

class Report {
public:
    void clear();
    void print_errstat();
    // error count
    uint32_t errstat_piderr; // USBx_ERRSTAT[PIDERR]
    uint32_t errstat_crc5eof;// USBx_ERRSTAT[CRC5EOF]
    uint32_t errstat_crc16;  // USBx_ERRSTAT[CRC16]
    uint32_t errstat_dfn8;   // USBx_ERRSTAT[DFN8]
    uint32_t errstat_btoerr; // USBx_ERRSTAT[BTOERR]
    uint32_t errstat_dmaerr; // USBx_ERRSTAT[DMAERR]
    uint32_t errstat_bsterr; // USBx_ERRSTAT[BTSERR]
    //
    uint32_t nak;
    uint32_t stall;
};

class USBHALHost {
public:
    uint8_t LastStatus;
    uint8_t prev_LastStatus;
    Report report;

protected:
    USBHALHost();
    void init();
    virtual bool addDevice(USBDeviceConnected* parent, int port, bool lowSpeed) = 0;
    int token_setup(USBEndpoint* ep, SETUP_PACKET* setup, uint16_t wLength = 0);
    int multi_token_in(USBEndpoint* ep, uint8_t* data = NULL, size_t total = 0, bool block = true);
    int multi_token_out(USBEndpoint* ep, const uint8_t* data = NULL, size_t total = 0);
    void multi_token_inNB(USBEndpoint* ep, uint8_t* data, int size);
    USB_TYPE multi_token_inNB_result(USBEndpoint* ep);
    void setToggle(USBEndpoint* ep, uint8_t toggle);
    int token_iso_in(USBEndpoint* ep, uint8_t* data, int size);

private:
    void setAddr(int addr, bool lowSpeed = false);
    void setEndpoint();
    void token_transfer_init();
    void token_ready();
    int token_in(USBEndpoint* ep, uint8_t* data = NULL, int size = 0, int retryLimit = 10);
    int token_out(USBEndpoint* ep, const uint8_t* data = NULL, int size = 0, int retryLimit = 10);
    static void _usbisr(void);
    void UsbIrqhandler();
    __IO bool attach_done;
    __IO bool token_done;
    bool wait_attach();
    bool root_lowSpeed;
    ODD_EVEN tx_ptr;
    ODD_EVEN rx_ptr;
    static USBHALHost * instHost;
};
