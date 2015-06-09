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
 
#pragma once
#include "mbed.h"
#include "rtos.h"
#include "USBHostTypes.h"
#include "USBEndpoint.h"

#if defined(TARGET_LPC4088)
#define HcRevision      Revision
#define HcControl       Control
#define HcCommandStatus CommandStatus
#define HcInterruptStatus InterruptStatus
#define HcInterruptEnable InterruptEnable
#define HcHCCA          HCCA
#define HcControlHeadED ControlHeadED
#define HcBulkHeadED    BulkHeadED
#define HcFmInterval    FmInterval
#define HcFmNumber      FmNumber
#define HcPeriodicStart PeriodicStart
#define HcRhStatus      RhStatus
#define HcRhPortStatus1 RhPortStatus1
#define OTGStCtrl       StCtrl
#endif
// ------------------ HcControl Register ---------------------
#define  OR_CONTROL_PLE                 0x00000004
#define  OR_CONTROL_IE                  0x00000008
#define  OR_CONTROL_CLE                 0x00000010
#define  OR_CONTROL_BLE                 0x00000020
#define  OR_CONTROL_HCFS                0x000000C0
#define  OR_CONTROL_HC_OPER             0x00000080
// ----------------- HcCommandStatus Register -----------------
#define  OR_CMD_STATUS_HCR              0x00000001
#define  OR_CMD_STATUS_CLF              0x00000002
#define  OR_CMD_STATUS_BLF              0x00000004
// --------------- HcInterruptStatus Register -----------------
#define  OR_INTR_STATUS_WDH             0x00000002
#define  OR_INTR_STATUS_UE              0x00000010
#define  OR_INTR_STATUS_FNO             0x00000020
#define  OR_INTR_STATUS_RHSC            0x00000040
// --------------- HcInterruptEnable Register -----------------
#define  OR_INTR_ENABLE_WDH             0x00000002
#define  OR_INTR_ENABLE_FNO             0x00000020
#define  OR_INTR_ENABLE_RHSC            0x00000040
#define  OR_INTR_ENABLE_MIE             0x80000000
// ---------------- HcRhDescriptorA Register ------------------
#define  OR_RH_STATUS_LPSC              0x00010000
#define  OR_RH_STATUS_DRWE              0x00008000
// -------------- HcRhPortStatus[1:NDP] Register --------------
#define  OR_RH_PORT_CCS                 0x00000001
#define  OR_RH_PORT_PRS                 0x00000010
#define  OR_RH_PORT_LSDA                0x00000200
#define  OR_RH_PORT_CSC                 0x00010000
#define  OR_RH_PORT_PRSC                0x00100000

// TRANSFER DESCRIPTOR CONTROL FIELDS
#define  TD_ROUNDING     (uint32_t)(0x00040000) /* Buffer Rounding */
#define  TD_SETUP        (uint32_t)(0x00000000) /* Direction of Setup Packet */
#define  TD_IN           (uint32_t)(0x00100000) /* Direction In */
#define  TD_OUT          (uint32_t)(0x00080000) /* Direction Out */
#define  TD_DELAY_INT(x) (uint32_t)((x) << 21)  /* Delay Interrupt */
#define  TD_DI           (uint32_t)(7<<21)      /* desable interrupt */
#define  TD_TOGGLE_0     (uint32_t)(0x02000000) /* Toggle 0 */
#define  TD_TOGGLE_1     (uint32_t)(0x03000000) /* Toggle 1 */
#define  TD_CC           (uint32_t)(0xF0000000) /* Completion Code */

void* usb_ram_malloc(size_t size, int aligment); // USBHALHost2_LPC4088.cpp
void usb_ram_free(void* p);

struct TBUF {
    uint8_t buf[0];
    TBUF(const void* data = NULL, int size = 0);
    void* operator new(size_t size, int buf_size) { return usb_ram_malloc(size+buf_size, 4); }
    void operator delete(void* p) { usb_ram_free(p); }
};

class HCED;

struct HCTD {    // HostController Transfer Descriptor
    __IO uint32_t Control;    // +0 Transfer descriptor control
    __IO uint8_t* CurrBufPtr; // +4 Physical address of current buffer pointer
    HCTD* Next;               // +8 Physical pointer to next Transfer Descriptor
    uint8_t*  BufEnd;         // +12 Physical address of end of buffer
    uint8_t* buf_top;         // +16 Buffer start address
    uint16_t buf_size;        // +20 buffer size size
    uint8_t _dummy[10];       // +22 dummy
    HCED* parent;             // +32 HCED object
                              // +36
    HCTD(HCED* ed);
    void* operator new(size_t size) { return usb_ram_malloc(size, 16); }
    void operator delete(void* p) { usb_ram_free(p); }

    void transfer(TBUF* tbuf, int len) {
        CurrBufPtr = tbuf->buf;
        buf_top = tbuf->buf;
        buf_size = len;
        BufEnd = const_cast<uint8_t*>(tbuf->buf)+len-1;
    }
    int getLengthTransferred() {
        if (CurrBufPtr) {
            return CurrBufPtr - buf_top;
        }
        return buf_size;
    }
    int status() {
        if (CurrBufPtr) {
            return CurrBufPtr - buf_top;
        }
        return buf_size;
    }
    
    uint8_t ConditionCode() {
        return Control>>28;
    }    
}; 

struct HCITD {    // HostController Isochronous Transfer Descriptor
    __IO uint32_t Control;      // +0 Transfer descriptor control
    uint8_t*  BufferPage0;      // +4 Buffer Page 0
    HCITD* Next;                // +8 Physical pointer to next Isochronous Transfer Descriptor
    uint8_t*  BufferEnd;        // +12 buffer End
    __IO uint16_t OffsetPSW[8]; // +16 Offset/PSW
    HCED* parent;               // +32 HCED object
    __IO uint8_t buf[0];        // +36 buffer
                                // +36
    HCITD(HCED* ed, uint16_t FrameNumber, int FrameCount, uint16_t PacketSize);
    uint8_t* Buffer(int fc);
    void* operator new(size_t size, int buf_size) { return usb_ram_malloc(size+buf_size, 32); }
    void operator delete(void* p) { usb_ram_free(p); }

    uint16_t StartingFrame() {
        return Control & 0xffff;
    }

    uint8_t FrameCount() {
        return ((Control>>24)&7)+1;
    }    

    uint8_t ConditionCode() {
        return Control>>28;
    }

    uint8_t ConditionCode(int fc) {
        uint16_t psw = OffsetPSW[fc];
        return psw>>12;
    }

    uint16_t Length(int fc) {
        uint16_t psw = OffsetPSW[fc];
        return psw & 0x7ff;
    }
};

#define HCTD_QUEUE_SIZE 3

struct HCED {    // HostController EndPoint Descriptor
    __IO uint32_t Control; // +0 Endpoint descriptor control
    HCTD* TailTd;          // +4 Physical address of tail in Transfer descriptor list
    __IO HCTD* HeadTd;     // +8 Physcial address of head in Transfer descriptor list
    HCED* Next;            // +12 Physical address of next Endpoint descriptor
                           // +16
    uint8_t m_ConditionCode;
    Queue<HCTD, HCTD_QUEUE_SIZE> _done_queue; // TD done queue
    struct {
        uint8_t queue_count;
        uint8_t queue_limit;
        uint16_t FrameNumber;
        uint8_t FrameCount; // 1-8
        HCITD* Current_itd;
        uint8_t Current_FrameCount;
    } iso;
    inline osEvent done_queue_get(uint32_t millisec) { return _done_queue.get(millisec); }
    inline void irqWdhHandler(HCTD* td) {_done_queue.put(td);} // WDH
    HCTD* get_queue_HCTD(uint32_t millisec=osWaitForever);
    HCITD* get_queue_HCITD();
    HCITD* new_HCITD();
    HCED(USBEndpoint* ep);
    void* operator new(size_t size) { return usb_ram_malloc(size, 16); }
    void operator delete(void* p) { usb_ram_free(p); }

    uint8_t FunctionAddress() {
        return Control & 0x7f;
    }

    uint8_t EndpointNumber() {
        return (Control>>7) & 0x7f;
    }

    int Speed() {
        return (Control>>13)&1;
    }

    void setFunctionAddress(int addr) {
        Control &= ~0x7f;
        Control |= addr;
    }

    void setMaxPacketSize(uint16_t size) {
        Control &= ~0x07ff0000;
        Control |= size<<16;
    }

    uint16_t getMaxPacketSize() {
        return (Control>>16)&0x7ff;
    }

    void setToggle(uint8_t toggle);
    uint8_t getToggle();

    int Skip() {
        return (Control>>14) & 1;
    }

    void setSkip() {
        Control |= (1<<14);
    }

    void setFormat() {
        Control |= (1<<15);
    }

    bool enqueue(HCTD* td);
    void init_queue(HCTD* td);
};

struct HCCA {    // Host Controller Communication Area
    HCED* InterruptTable[32]; // +0 Interrupt Table
    __IO uint16_t FrameNumber;// +128 Frame Number
    __IO uint16_t Pad1;       // +130
    __IO HCTD* DoneHead;      // +132 Done Head
    uint8_t Reserved[116];    // +136 Reserved for future use
    uint8_t Unknown[4];       // +252 Unused
                              // +256
    void* operator new(size_t size) { return usb_ram_malloc(size, 256); }
    void operator delete(void* p) { usb_ram_free(p); }
    void enqueue(HCED* ed);
};

class USBHALHost {
protected:
    USBHALHost();
    void init();
    virtual bool addDevice(USBDeviceConnected* parent, int port, bool lowSpeed) = 0;
    int token_setup(USBEndpoint* ep, SETUP_PACKET* setup, uint16_t wLength = 0);
    int token_iso_in(USBEndpoint* ep, uint8_t* data, int size);
    int multi_token_in(USBEndpoint* ep, uint8_t* data = NULL, int size = 0);
    int multi_token_out(USBEndpoint* ep, const uint8_t* data = NULL, int size = 0);
    void multi_token_inNB(USBEndpoint* ep, uint8_t* data, int size);
    USB_TYPE multi_token_inNB_result(USBEndpoint* ep);
    void setToggle(USBEndpoint* ep, uint8_t toggle);

    // report
    uint32_t m_report_irq; 
    uint32_t m_report_RHSC;
    uint32_t m_report_FNO; 
    uint32_t m_report_WDH;  
    uint32_t m_report_sp;

private:
    void init_hw_ohci(HCCA* pHcca);
    void ResetRootHub();
    void token_init(USBEndpoint* ep);
    static void _usbisr(void);
    void UsbIrqhandler();
    HCCA* m_pHcca;
    bool wait_attach();
    bool root_lowSpeed;
    static USBHALHost * instHost;
};

