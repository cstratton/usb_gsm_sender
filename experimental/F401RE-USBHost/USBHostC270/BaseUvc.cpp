// BaseUvc.cpp
#include "USBHostConf.h"
#include "USBHost.h"
#include "BaseUvc.h"

void BaseUvc::poll() {
    uint8_t buf[ep_iso_in->getSize()];
    int result = host->isochronousReadNB(ep_iso_in, buf, sizeof(buf));
    if (result >= 0) {
        const uint16_t frame = 0;
        onResult(frame, buf, ep_iso_in->getLengthTransferred());
    }
}

USB_TYPE BaseUvc::Control(int req, int cs, int index, uint8_t* buf, int size) {
    if (req == SET_CUR) {    
        return host->controlWrite(dev,
                    USB_HOST_TO_DEVICE | USB_REQUEST_TYPE_CLASS | USB_RECIPIENT_INTERFACE, 
                    req, cs<<8, index, buf, size);
    }
    return host->controlRead(dev,
                USB_DEVICE_TO_HOST | USB_REQUEST_TYPE_CLASS | USB_RECIPIENT_INTERFACE, 
                req, cs<<8, index, buf, size);
}

USB_TYPE BaseUvc::setInterfaceAlternate(uint8_t intf, uint8_t alt) {
    return host->controlWrite(dev, USB_HOST_TO_DEVICE | USB_RECIPIENT_INTERFACE,
                                   SET_INTERFACE, alt, intf, NULL, 0);
}

void BaseUvc::onResult(uint16_t frame, uint8_t* buf, int len)
{
  if(m_pCbItem && m_pCbMeth)
    (m_pCbItem->*m_pCbMeth)(frame, buf, len);
  else if(m_pCb)
    m_pCb(frame, buf, len);
}

void BaseUvc::setOnResult( void (*pMethod)(uint16_t, uint8_t*, int) )
{
    m_pCb = pMethod;
    m_pCbItem = NULL;
    m_pCbMeth = NULL;
}
    
void BaseUvc::clearOnResult()
{
    m_pCb = NULL;
    m_pCbItem = NULL;
    m_pCbMeth = NULL;
}

