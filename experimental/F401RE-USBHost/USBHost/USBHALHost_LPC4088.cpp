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

#if defined(TARGET_LPC4088)||defined(TARGET_LPC1768)
#include "USBHALHost.h"

#ifndef CTASSERT
template <bool>struct CtAssert;
template <>struct CtAssert<true> {};
#define CTASSERT(A) CtAssert<A>();
#endif

#ifdef _USB_DBG
#define USB_DBG(...) do{fprintf(stderr,"[%s@%d] ",__PRETTY_FUNCTION__,__LINE__);fprintf(stderr,__VA_ARGS__);fprintf(stderr,"\n");} while(0);
#define USB_DBG_HEX(A,B) debug_hex(A,B)
#define USB_DBG_ED(A) while(0)
#define USB_DBG_TD(A) while(0)
#define USB_DBG_ED_IF(A,B) while(0)
void debug_hex(uint8_t* buf, int size);
#else
#define USB_DBG(...) while(0)
#define USB_DBG_ED(A) while(0)
#define USB_DBG_TD(A) while(0)
#define USB_DBG_ED_IF(A,B) while(0)
#define USB_DBG_HEX(A,B) while(0)
#endif

#define USB_TRACE1(A) while(0)

#ifdef _USB_TEST
#undef USB_TEST_ASSERT
void usb_test_assert_internal(const char *expr, const char *file, int line);
#define USB_TEST_ASSERT(EXPR) while(!(EXPR)){usb_test_assert_internal(#EXPR,__FILE__,__LINE__);}
#else
#define USB_TEST_ASSERT(EXPR) while(0)
#endif

#define USB_INFO(...) do{fprintf(stderr,__VA_ARGS__);fprintf(stderr,"\n");}while(0);

// bits of the USB/OTG clock control register
#define HOST_CLK_EN     (1<<0)
#define DEV_CLK_EN      (1<<1)
#define PORTSEL_CLK_EN  (1<<3)
#define AHB_CLK_EN      (1<<4)

// bits of the USB/OTG clock status register
#define HOST_CLK_ON     (1<<0)
#define DEV_CLK_ON      (1<<1)
#define PORTSEL_CLK_ON  (1<<3)
#define AHB_CLK_ON      (1<<4)

// we need host clock, OTG/portsel clock and AHB clock
#define CLOCK_MASK (HOST_CLK_EN | PORTSEL_CLK_EN | AHB_CLK_EN)
#define  FI                     0x2EDF           /* 12000 bits per frame (-1) */
#define  DEFAULT_FMINTERVAL     ((((6 * (FI - 210)) / 7) << 16) | FI)

USBHALHost* USBHALHost::instHost;

USBHALHost::USBHALHost() {
    instHost = this;
}

void USBHALHost::init() {
    NVIC_DisableIRQ(USB_IRQn);
    m_pHcca = new HCCA();
    init_hw_ohci(m_pHcca);
    ResetRootHub();
    NVIC_SetVector(USB_IRQn, (uint32_t)_usbisr);
    NVIC_SetPriority(USB_IRQn, 0);
    NVIC_EnableIRQ(USB_IRQn);

    USB_INFO("Simple USBHost Library for LPC4088/LPC1768");
    bool lowSpeed = wait_attach();
    addDevice(NULL, 0, lowSpeed);
}

void USBHALHost::init_hw_ohci(HCCA* pHcca) {
    LPC_SC->PCONP &= ~(1UL<<31);    //Cut power
    wait_ms(1000);
    LPC_SC->PCONP |= (1UL<<31);     // turn on power for USB
    LPC_USB->USBClkCtrl |= CLOCK_MASK; // Enable USB host clock, port selection and AHB clock
    // Wait for clocks to become available
    while ((LPC_USB->USBClkSt & CLOCK_MASK) != CLOCK_MASK)
        ;
    LPC_USB->OTGStCtrl |= 1;
    LPC_USB->USBClkCtrl &= ~PORTSEL_CLK_EN;
    
#if defined(TARGET_LPC1768)
    LPC_PINCON->PINSEL1 &= ~((3<<26) | (3<<28));  
    LPC_PINCON->PINSEL1 |=  ((1<<26)|(1<<28));     // 0x14000000
#elif defined(TARGET_LPC4088)
    LPC_IOCON->P0_29 = 0x01; // USB_D+1
    LPC_IOCON->P0_30 = 0x01; // USB_D-1
    // DO NOT CHANGE P1_19.
#else
#error "target error"
#endif
    USB_DBG("initialize OHCI\n");
    wait_ms(100);                                   /* Wait 50 ms before apply reset              */
    USB_TEST_ASSERT((LPC_USB->HcRevision&0xff) == 0x10); // check revision
    LPC_USB->HcControl       = 0;                       /* HARDWARE RESET                             */
    LPC_USB->HcControlHeadED = 0;                       /* Initialize Control list head to Zero       */
    LPC_USB->HcBulkHeadED    = 0;                       /* Initialize Bulk list head to Zero          */
                                                        /* SOFTWARE RESET                             */
    LPC_USB->HcCommandStatus = OR_CMD_STATUS_HCR;
    LPC_USB->HcFmInterval    = DEFAULT_FMINTERVAL;      /* Write Fm Interval and Largest Data Packet Counter */
    LPC_USB->HcPeriodicStart = FI*90/100;
                                                      /* Put HC in operational state                */
    LPC_USB->HcControl  = (LPC_USB->HcControl & (~OR_CONTROL_HCFS)) | OR_CONTROL_HC_OPER;
    LPC_USB->HcRhStatus = OR_RH_STATUS_LPSC;            /* Set Global Power */
    USB_TEST_ASSERT(pHcca);
    for (int i = 0; i < 32; i++) {
        pHcca->InterruptTable[i] = NULL;
    }
    LPC_USB->HcHCCA = reinterpret_cast<uint32_t>(pHcca);
    LPC_USB->HcInterruptStatus |= LPC_USB->HcInterruptStatus; /* Clear Interrrupt Status */
    LPC_USB->HcInterruptEnable  = OR_INTR_ENABLE_MIE|OR_INTR_ENABLE_WDH|OR_INTR_ENABLE_FNO;

    LPC_USB->HcRhPortStatus1 = OR_RH_PORT_CSC;
    LPC_USB->HcRhPortStatus1 = OR_RH_PORT_PRSC;
}

void USBHALHost::ResetRootHub() {
    wait_ms(100); /* USB 2.0 spec says at least 50ms delay before port reset */
    LPC_USB->HcRhPortStatus1 = OR_RH_PORT_PRS; // Initiate port reset
    USB_DBG("Before loop\n");
    while (LPC_USB->HcRhPortStatus1 & OR_RH_PORT_PRS)
      ;
    LPC_USB->HcRhPortStatus1 = OR_RH_PORT_PRSC; // ...and clear port reset signal
    USB_DBG("After loop\n");
    wait_ms(200); /* Wait for 100 MS after port reset  */
}

bool USBHALHost::wait_attach() {
    bool lowSpeed = false;
    uint32_t status = LPC_USB->HcRhPortStatus1;
    if (status & OR_RH_PORT_LSDA) { // lowSpeedDeviceAttached
        lowSpeed = true;
    }
    return lowSpeed;
}

void enable(ENDPOINT_TYPE type) {
    switch(type) {
        case CONTROL_ENDPOINT:
            LPC_USB->HcCommandStatus |= OR_CMD_STATUS_CLF;
            LPC_USB->HcControl |= OR_CONTROL_CLE;
            break;
        case ISOCHRONOUS_ENDPOINT:
            LPC_USB->HcControl |= OR_CONTROL_PLE;
            break;
        case BULK_ENDPOINT:
            LPC_USB->HcCommandStatus |= OR_CMD_STATUS_BLF;
            LPC_USB->HcControl |= OR_CONTROL_BLE;
            break;
        case INTERRUPT_ENDPOINT:
            LPC_USB->HcControl |= OR_CONTROL_PLE;
            break;
    }
}

void USBHALHost::token_init(USBEndpoint* ep) {
    HCED* ed = ep->getHALData<HCED*>();
    if (ed == NULL) {
        ed = new HCED(ep);
        ep->setHALData<HCED*>(ed);
    }
    USBDeviceConnected* dev = ep->getDevice();
    USB_TEST_ASSERT(dev);
    if (dev) {
        uint8_t devAddr = dev->getAddress();
        USB_DBG("devAddr=%02x", devAddr);
        ed->setFunctionAddress(devAddr);
    }
    uint16_t size = ep->getSize();
    USB_DBG("MaxPacketSize=%d", size);
    ed->setMaxPacketSize(size);
    if (ed->HeadTd == NULL) {
        HCTD* td = new HCTD(ed);
        ed->TailTd = td;
        ed->HeadTd = td;
        switch(ep->getType()) {
            case CONTROL_ENDPOINT:
                ed->Next = reinterpret_cast<HCED*>(LPC_USB->HcControlHeadED);
                LPC_USB->HcControlHeadED = reinterpret_cast<uint32_t>(ed);
                break;
            case BULK_ENDPOINT:
                ed->Next = reinterpret_cast<HCED*>(LPC_USB->HcBulkHeadED);
                LPC_USB->HcBulkHeadED = reinterpret_cast<uint32_t>(ed);
                break;
            case INTERRUPT_ENDPOINT:
                HCCA* pHcca = reinterpret_cast<HCCA*>(LPC_USB->HcHCCA);
                ed->Next = pHcca->InterruptTable[0];
                pHcca->InterruptTable[0] = ed;
                break;
        }
    }
}

int USBHALHost::token_setup(USBEndpoint* ep, SETUP_PACKET* setup, uint16_t wLength) {
    token_init(ep);
    HCED* ed = ep->getHALData<HCED*>();
    HCTD* td = ed->TailTd;
    setup->wLength = wLength;
    TBUF* tbuf = new(sizeof(SETUP_PACKET))TBUF(setup, sizeof(SETUP_PACKET));
    td->transfer(tbuf, sizeof(SETUP_PACKET));
    td->Control |= TD_TOGGLE_0|TD_SETUP; // DATA0
    HCTD* blank = new HCTD(ed);
    ed->enqueue(blank);
    //DBG_ED(ed);
    enable(ep->getType());

    td = ed->get_queue_HCTD();
    USB_TEST_ASSERT(td);
    int result = td->getLengthTransferred();
    USB_DBG_TD(td);
    delete tbuf;
    delete td;
    return result;
}

static HCED* getHCED_iso(USBEndpoint* ep) {
    HCED* ed = ep->getHALData<HCED*>();
    if (ed != NULL) {
        return ed;
    }
    ed = new HCED(ep);
    ep->setHALData<HCED*>(ed);
    ed->setFormat(); // F Format ITD
    ed->iso.FrameCount = ep->ohci.frameCount;
    ed->iso.queue_limit = ep->ohci.queueLimit;
    ed->iso.queue_count = 0;
    ed->iso.Current_FrameCount = 0;
    ed->iso.Current_itd = NULL;
    ed->iso.FrameNumber = LPC_USB->HcFmNumber + 10; // after 10msec
    HCITD* itd = ed->new_HCITD();
    ed->init_queue(reinterpret_cast<HCTD*>(itd)); 
    HCCA* hcca = reinterpret_cast<HCCA*>(LPC_USB->HcHCCA);
    hcca->enqueue(ed);
    LPC_USB->HcControl |= OR_CONTROL_PLE; // PeriodicListEnable
    LPC_USB->HcControl |= OR_CONTROL_IE;  // IsochronousEnable
    return ed;
}

static void enablePeriodic() {
    LPC_USB->HcControl |= OR_CONTROL_PLE;
}

HCITD* isochronousReceive(USBEndpoint* ep) {
    HCED* ed = getHCED_iso(ep);
    USB_TEST_ASSERT(ed);
    while(ed->iso.queue_count < ed->iso.queue_limit) {
        HCITD* blank_itd = ed->new_HCITD();
        if (ed->enqueue(reinterpret_cast<HCTD*>(blank_itd))) {
            ed->iso.queue_count++;
        }
        enablePeriodic();
    }
    
    HCITD* itd = ed->get_queue_HCITD();
    if (itd) {
        ed->iso.queue_count--;
    }
    return itd;
}

int USBHALHost::token_iso_in(USBEndpoint* ep, uint8_t* data, int size) {
    HCED* ed = getHCED_iso(ep);
    if (ed->iso.Current_FrameCount == 0) {
        HCITD* itd = isochronousReceive(ep);
        if (itd == NULL) {
            return -1;
        }
        if (itd->ConditionCode() != 0) {
            delete itd;
            return -1;
        }
        ed->iso.Current_itd = itd;
    }

    HCITD* itd = ed->iso.Current_itd;
    int fc = ed->iso.Current_FrameCount;
    int result = -1;
    uint8_t cc = itd->ConditionCode(fc);
    if (cc == 0 || cc == 9) {
        result = itd->Length(fc);
        memcpy(data, itd->Buffer(fc), result);
    }

    if (++ed->iso.Current_FrameCount >= itd->FrameCount()) {
        ed->iso.Current_FrameCount = 0;
        delete ed->iso.Current_itd;
    }
    return result;
}

int USBHALHost::multi_token_in(USBEndpoint* ep, uint8_t* data, int size) {
    token_init(ep);
    HCED* ed = ep->getHALData<HCED*>();
    HCTD* td = ed->TailTd;
    TBUF* tbuf = NULL;
    if (data != NULL) {
        tbuf = new(size)TBUF();
        td->transfer(tbuf, size);
    }
    td->Control |= TD_IN;
    HCTD* blank = new HCTD(ed);
    ed->enqueue(blank);
    USB_DBG_ED_IF(ed->EndpointNumber()==0, ed); // control transfer
    enable(ep->getType());

    td = ed->get_queue_HCTD();
    USB_TEST_ASSERT(td);
    if (data != NULL) {
        memcpy(data, tbuf->buf, size);
        delete tbuf;
    }
    int result = td->getLengthTransferred();
    delete td;
    return result;
}

int USBHALHost::multi_token_out(USBEndpoint* ep, const uint8_t* data, int size) {
    token_init(ep);
    HCED* ed = ep->getHALData<HCED*>();
    HCTD* td = ed->TailTd;
    TBUF* tbuf = NULL;
    if (data != NULL) {
        tbuf = new(size)TBUF(data, size);
        td->transfer(tbuf, size);
    }
    td->Control |= TD_OUT;
    HCTD* blank = new HCTD(ed);
    ed->enqueue(blank);
    USB_DBG_ED(ed);
    enable(ep->getType());

    td = ed->get_queue_HCTD();
    USB_TEST_ASSERT(td);
    if (data != NULL) {
        delete tbuf;
    }
    int result = td->getLengthTransferred();
    delete td;
    return result;
}

void USBHALHost::multi_token_inNB(USBEndpoint* ep, uint8_t* data, int size) {
    token_init(ep);
    HCED* ed = ep->getHALData<HCED*>();
    HCTD* td = ed->TailTd;
    TBUF* tbuf = new(size)TBUF();
    td->transfer(tbuf, size);
    td->Control |= TD_IN;
    HCTD* blank = new HCTD(ed);
    ed->enqueue(blank);
    enable(ep->getType());
}

USB_TYPE USBHALHost::multi_token_inNB_result(USBEndpoint* ep) {
    HCED* ed = ep->getHALData<HCED*>();
    if (ed == NULL) {
        return USB_TYPE_ERROR;
    }
    HCTD* td = ed->get_queue_HCTD(0);
    if (td) {
        int len = td->getLengthTransferred();
        TBUF* tbuf = (TBUF*)td->buf_top;
        memcpy(ep->getBufStart(), tbuf->buf, len);
        ep->setLengthTransferred(len);
        ep->setState((USB_TYPE)td->ConditionCode());
        delete td;
        delete tbuf;
        return USB_TYPE_OK;
    }
    return USB_TYPE_PROCESSING;
}

void USBHALHost::setToggle(USBEndpoint* ep, uint8_t toggle) {
    USB_TEST_ASSERT(toggle == 1);
    HCED* ed = ep->getHALData<HCED*>();
    ed->setToggle(toggle);
}

void USBHALHost::_usbisr(void) {
    if (instHost) {
        instHost->UsbIrqhandler();
    }
}

HCTD* td_reverse(HCTD* td)
{
    HCTD* result = NULL;
    HCTD* next;
    while(td) {
        next = const_cast<HCTD*>(td->Next);
        td->Next = result;
        result = td;
        td = next;
    }
    return result;
}

void USBHALHost::UsbIrqhandler() {
    if (!(LPC_USB->HcInterruptStatus & LPC_USB->HcInterruptEnable)) {
        return;
    }
    m_report_irq++;
    uint32_t status = LPC_USB->HcInterruptStatus;
    if (status & OR_INTR_STATUS_FNO) {
        m_report_FNO++;
    }
    if (status & OR_INTR_STATUS_WDH) {
        union {
            HCTD* done_td;
            uint32_t lsb;
        };
        done_td = const_cast<HCTD*>(m_pHcca->DoneHead);
        USB_TEST_ASSERT(done_td);
        m_pHcca->DoneHead = NULL; // reset
        if (lsb & 1) { // error ?
            lsb &= ~1;
        }
        HCTD* td = td_reverse(done_td);
        while(td) {
            HCED* ed = td->parent;
            USB_TEST_ASSERT(ed);
            if (ed) {
                ed->irqWdhHandler(td);
            }
            td = td->Next;
        }
        m_report_WDH++;
    }
    LPC_USB->HcInterruptStatus = status; // Clear Interrrupt Status
}

TBUF::TBUF(const void* data, int size) {
    if (size > 0) {
        memcpy(buf, data, size);
    }
}

HCTD::HCTD(HCED* obj) {
    CTASSERT(sizeof(HCTD) == 36);
    USB_TEST_ASSERT(obj);
    Control = TD_CC|TD_ROUNDING;
    CurrBufPtr = NULL;
    Next = NULL;
    BufEnd = NULL;
    buf_top = NULL;
    buf_size = 0;
    parent = obj;
}

HCITD::HCITD(HCED* obj, uint16_t FrameNumber, int FrameCount, uint16_t PacketSize) {
    Control = 0xe0000000           | // CC ConditionCode NOT ACCESSED
             ((FrameCount-1) << 24)| // FC FrameCount
                  TD_DELAY_INT(0)  | // DI DelayInterrupt
                 FrameNumber;        // SF StartingFrame
    BufferPage0 = const_cast<uint8_t*>(buf);
    BufferEnd = const_cast<uint8_t*>(buf) + PacketSize * FrameCount - 1;
    Next = NULL; 
    parent = obj;
    uint32_t addr = reinterpret_cast<uint32_t>(buf);
    for(int i = 0; i < FrameCount; i++) {
        uint16_t offset = addr & 0x0fff;
        if ((addr&0xfffff000) == (reinterpret_cast<uint32_t>(BufferEnd)&0xfffff000)) {
            offset |= 0x1000;
        }
        OffsetPSW[i] = 0xe000|offset;
        addr += PacketSize;
    }
}

uint8_t* HCITD::Buffer(int fc) {
    int offset = fc * parent->getMaxPacketSize();
    return const_cast<uint8_t*>(buf) + offset;
}

HCED::HCED(USBEndpoint* ep) {
    CTASSERT(sizeof(HCED) <= (64*2));
    USBDeviceConnected* dev = ep->getDevice();
    int devAddr = 0;
    bool lowSpeed = false;
    if (dev) {
        devAddr = dev->getAddress();
        lowSpeed = dev->getSpeed();
    }
    int ep_number = ep->getAddress();
    int MaxPacketSize = ep->getSize();
        Control =  devAddr        | /* USB address */
        ((ep_number & 0x7F) << 7) | /* Endpoint address */
        (ep_number!=0?(((ep_number&0x80)?2:1) << 11):0)| /* direction : Out = 1, 2 = In */
        ((lowSpeed?1:0) << 13)    | /* speed full=0 low=1 */
        (MaxPacketSize << 16);      /* MaxPkt Size */
        TailTd = NULL;
        HeadTd = NULL;
        Next = NULL;
}

bool HCED::enqueue(HCTD* td) {
    if (td) {
        HCTD* tail = reinterpret_cast<HCTD*>(TailTd);
        if (tail) {
            tail->Next = td;
            TailTd = reinterpret_cast<HCTD*>(td);
            return true;
        }
    }
    return false;
}

void HCED::init_queue(HCTD* td) {
    TailTd = reinterpret_cast<HCTD*>(td);
    HeadTd = reinterpret_cast<HCTD*>(td); 
}

void HCED::setToggle(uint8_t toggle) {
    uint32_t c = reinterpret_cast<uint32_t>(HeadTd);
    if (toggle == 0) { // DATA0
        c &= ~0x02;
    } else { // DATA1
        c |= 0x02;
    }
    HeadTd = reinterpret_cast<HCTD*>(c);
}

uint8_t HCED::getToggle() {
    uint32_t c = reinterpret_cast<uint32_t>(HeadTd);
    return (c&0x02) ? 1 : 0;
}

HCTD* HCED::get_queue_HCTD(uint32_t millisec)
{
    for(int i = 0; i < 16; i++) {
        osEvent evt = done_queue_get(millisec);
        if (evt.status == osEventMessage) {
            HCTD* td = reinterpret_cast<HCTD*>(evt.value.p);
            USB_TEST_ASSERT(td);
            uint8_t cc = td->ConditionCode();
            if (cc != 0) {
                m_ConditionCode = cc;
                USB_DBG_TD(td);
            }
            return td;
        } else if (evt.status == osOK) {
            continue;
        } else if (evt.status == osEventTimeout) {
            return NULL;
        } else {
            USB_DBG("evt.status: %02x\n", evt.status);
            USB_TEST_ASSERT(evt.status == osEventMessage);
        }
    }
    return NULL;
}

HCITD* HCED::get_queue_HCITD() {
    osEvent evt = done_queue_get(0);
    if (evt.status == osEventMessage) {
        HCITD* itd = reinterpret_cast<HCITD*>(evt.value.p);
        USB_TEST_ASSERT(itd);
        return itd;
    }
    return NULL;
}
HCITD* HCED::new_HCITD() {
    uint16_t mps = getMaxPacketSize();
    int total_size = mps * iso.FrameCount;
    HCITD* itd = new(total_size)HCITD(this, iso.FrameNumber, iso.FrameCount, mps);
    iso.FrameNumber += iso.FrameCount;
    return itd;
}

void HCCA::enqueue(HCED* ed) {
    for(int i = 0; i < 32; i++) {
        if (InterruptTable[i] == NULL) {
            InterruptTable[i] = ed;
        } else {
            HCED* nextEd = InterruptTable[i];
            while(nextEd->Next && nextEd->Next != ed) {
                nextEd = nextEd->Next;
            }
            nextEd->Next = ed;
        }
    }
}

#endif // defined(TARGET_LPC4088)||defined(TARGET_LPC1768)

