/* mbed USBHost Library
 * Copyright (c) 2006-2013 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef USBHOSTKEYBOARD_H
#define USBHOSTKEYBOARD_H

#include "USBHostConf.h"

#if USBHOST_KEYBOARD

#include "USBHost.h"

/** 
 * A class to communicate a USB keyboard
 */
class USBHostE303 : public IUSBEnumerator {
public:
    
    /**
    * Constructor
    */
    USBHostE303();

    /**
     * Try to connect a keyboard device
     *
     * @return true if connection was successful
     */
    bool connect();

    /**
    * Check if a keyboard is connected
    *
    * @returns true if a keyboard is connected
    */
    bool connected();

    /**
     * Attach a callback called when a keyboard event is received
     *
     * @param ptr function pointer
     */
    inline void attach(void (*ptr)(uint8_t key)) {
        if (ptr != NULL) {
            onKey = ptr;
        }
    }

    /**
     * Attach a callback called when a keyboard event is received
     *
     * @param ptr function pointer
     */
    inline void attach(void (*ptr)(uint8_t keyCode, uint8_t modifier)) {
        if (ptr != NULL) {
            onKeyCode = ptr;
        }
    }

    int doswitch();
    int bulk_write(void *data, int len);
    int bulk_read(void *data, int len);


protected:
    //From IUSBEnumerator
    virtual void setVidPid(uint16_t vid, uint16_t pid);
    virtual bool parseInterface(uint8_t intf_nb, uint8_t intf_class, uint8_t intf_subclass, uint8_t intf_protocol); //Must return true if the interface should be parsed
    virtual bool useEndpoint(uint8_t intf_nb, ENDPOINT_TYPE type, ENDPOINT_DIRECTION dir); //Must return true if the endpoint will be used

private:
    USBHost * host;
    USBDeviceConnected * dev;
    USBEndpoint * bulk_in;
    USBEndpoint * bulk_out;
    uint8_t report[9];
    int keyboard_intf;
    bool preswitch_device_found;
    bool postswitch_device_found;

    bool dev_connected;

    void rxHandler();

    void (*onKey)(uint8_t key);
    void (*onKeyCode)(uint8_t key, uint8_t modifier);

    int report_id;
    
    void init();
    int checkResult(uint8_t res, USBEndpoint * ep);

};

#endif

#endif
