// BaseUvc.h
#pragma once

// --- UVC --------------------------------------------------
#define _30FPS  333333
#define _25FPS  400000
#define _20FPS  500000
#define _15FPS  666666
#define _10FPS 1000000
#define _5FPS  2000000
#define _1FPS 10000000

#define SET_CUR  0x01
#define GET_CUR  0x81
#define GET_MIN  0x82
#define GET_MAX  0x83
#define GET_RES  0x84
#define GET_LEN  0x85
#define GET_INFO 0x86
#define GET_DEF  0x87

#define VS_PROBE_CONTROL  0x01
#define VS_COMMIT_CONTROL 0x02

class BaseUvc {
public:
    void poll();
    USB_TYPE Control(int req, int cs, int index, uint8_t* buf, int size);
    USB_TYPE setInterfaceAlternate(uint8_t intf, uint8_t alt);
    //IsochronousEp* m_isoEp;
    // callback
    void onResult(uint16_t frame, uint8_t* buf, int len);
    void setOnResult( void (*pMethod)(uint16_t, uint8_t*, int) );
    class CDummy;
    template<class T> 
    void setOnResult( T* pItem, void (T::*pMethod)(uint16_t, uint8_t*, int) )
    {
        m_pCb = NULL;
        m_pCbItem = (CDummy*) pItem;
        m_pCbMeth = (void (CDummy::*)(uint16_t, uint8_t*, int)) pMethod;
    }
    void clearOnResult();
    CDummy* m_pCbItem;
    void (CDummy::*m_pCbMeth)(uint16_t, uint8_t*, int);
    void (*m_pCb)(uint16_t, uint8_t*, int);
protected:
    USBHost * host;
    USBDeviceConnected * dev;
    USBEndpoint* ep_iso_in;
};
