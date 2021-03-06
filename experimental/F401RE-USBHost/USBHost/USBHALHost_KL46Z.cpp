// Simple USBHost for FRDM-KL46Z
#if defined(TARGET_KL46Z)||defined(TARGET_KL25Z)||defined(TARGET_K64F) ||defined(TARGET_K20D50M) || defined(TARGET_TEENSY3_1)
#include "USBHALHost.h"
#include <algorithm>

#if 1 //def _USB_DBG
#define USB_DBG(...) do{fprintf(stderr,"[%s@%d] ",__PRETTY_FUNCTION__,__LINE__);fprintf(stderr,__VA_ARGS__);fprintf(stderr,"\n");} while(0);
#define USB_DBG_HEX(A,B) debug_hex(A,B)
void debug_hex(uint8_t* buf, int size);
#else
#define USB_DBG(...) while(0)
#define USB_DBG_HEX(A,B) while(0)
#endif

#undef USB_TEST_ASSERT
extern void usb_test_assert_internal(const char *expr, const char *file, int line);
#define USB_TEST_ASSERT(EXPR) while(!(EXPR)){usb_test_assert_internal(#EXPR,__FILE__,__LINE__);}

#define USB_TRACE1(A) while(0)

#define USB_INFO(...) do{fprintf(stderr,__VA_ARGS__);fprintf(stderr,"\n");}while(0);

#define BD_OWN_MASK        (1<<7)
#define BD_DATA01_MASK     (1<<6)
#define BD_KEEP_MASK       (1<<5)
#define BD_NINC_MASK       (1<<4)
#define BD_DTS_MASK        (1<<3)
#define BD_STALL_MASK      (1<<2)

#define TX    1
#define RX    0

#define EP0_BDT_IDX(dir, odd) (((2 * dir) + (1 * odd)))

#define SETUP_TOKEN    0x0D
#define IN_TOKEN       0x09
#define OUT_TOKEN      0x01

// for each endpt: 8 bytes
struct BDT {
    uint8_t   info;       // BD[0:7]
    uint8_t   dummy;      // RSVD: BD[8:15]
    uint16_t  byte_count; // BD[16:32]
    uint32_t  address;    // Addr
    void setBuffer(uint8_t* buf, int size) {
        address = (uint32_t)buf;
        byte_count = size;
    }
    uint8_t getStatus() {
        return (info>>2)&0x0f;
    }    
};

__attribute__((__aligned__(512))) BDT bdt[64];

USBHALHost* USBHALHost::instHost;

USBHALHost::USBHALHost() {
    instHost = this;
    report.clear();
}

void USBHALHost::init() {
    // Disable IRQ
    NVIC_DisableIRQ(USB0_IRQn);

#if defined(TARGET_K64F)
    MPU->CESR=0;
#endif

    // choose usb src as PLL
    SIM->SOPT2 |= (SIM_SOPT2_USBSRC_MASK | SIM_SOPT2_PLLFLLSEL_MASK);

    // enable OTG clock
    SIM->SCGC4 |= SIM_SCGC4_USBOTG_MASK;

    // USB Module Configuration
    // Reset USB Module
    USB0->USBTRC0 |= USB_USBTRC0_USBRESET_MASK;
    while(USB0->USBTRC0 & USB_USBTRC0_USBRESET_MASK);

    // Clear interrupt flag
    USB0->ISTAT = 0xff;

    // Set BDT Base Register
    USB0->BDTPAGE1=(uint8_t)((uint32_t)bdt>>8);
    USB0->BDTPAGE2=(uint8_t)((uint32_t)bdt>>16);
    USB0->BDTPAGE3=(uint8_t)((uint32_t)bdt>>24);

    // Set SOF threshold
    USB0->SOFTHLD = USB_SOFTHLD_CNT(1);

    // pulldown D+ and D-
    USB0->USBCTRL = USB_USBCTRL_PDE_MASK;

    USB0->USBTRC0 |= 0x40;

    // Host mode
    USB0->CTL |= USB_CTL_HOSTMODEEN_MASK;
    // Desable SOF packet generation
    USB0->CTL &= ~USB_CTL_USBENSOFEN_MASK;

    NVIC_SetVector(USB0_IRQn, (uint32_t)_usbisr);
    NVIC_EnableIRQ(USB0_IRQn);

    bool lowSpeed = wait_attach();

    for(int retry = 2; retry > 0; retry--) {
        // Enable RESET
        USB0->CTL |= USB_CTL_RESET_MASK;
        wait_ms(500);
        USB0->CTL &= ~USB_CTL_RESET_MASK;
    
        // Enable SOF
        USB0->CTL |= USB_CTL_USBENSOFEN_MASK;
        wait_ms(100);

        // token transfer initialize
        token_transfer_init();

        USB0->INTEN |= USB_INTEN_TOKDNEEN_MASK|
                       USB_INTEN_ERROREN_MASK;
        USB0->ERREN |= USB_ERREN_PIDERREN_MASK|
                       USB_ERREN_CRC5EOFEN_MASK|
                       USB_ERREN_CRC16EN_MASK|
                       USB_ERREN_DFN8EN_MASK|
                       USB_ERREN_BTOERREN_MASK|
                       USB_ERREN_DMAERREN_MASK|
                       USB_ERREN_BTSERREN_MASK;

        if (addDevice(NULL, 0, lowSpeed)) {
            break;
        }
        USB_DBG("retry=%d", retry);
        USB_TEST_ASSERT(retry > 1);
    }
}

bool USBHALHost::wait_attach() {
    attach_done = false;
    USB0->INTEN = USB_INTEN_ATTACHEN_MASK;
    Timer t;
    t.reset();
    t.start();
    while(!attach_done) {
        if (t.read_ms() > 5*1000) {
            t.reset();
            USB_INFO("Please attach USB device.");
        }
    }
    wait_ms(100);
    while(USB0->CTL & USB_CTL_SE0_MASK) {
      USB_INFO("wiating for SEO to go away\n");
      wait_ms(1000);
    }
    USB_TEST_ASSERT(!(USB0->CTL & USB_CTL_SE0_MASK));
    root_lowSpeed = (USB0->CTL & USB_CTL_JSTATE_MASK) ? false : true;
    printf("device is lowspeed %d\n", root_lowSpeed);
    return root_lowSpeed;
}

void USBHALHost::setAddr(int _addr, bool _lowSpeed) {
  printf("USB0->ADDR = %02x\n", (_lowSpeed ? USB_ADDR_LSEN_MASK : 0x00) | USB_ADDR_ADDR(_addr));
  USB0->ADDR = (_lowSpeed ? USB_ADDR_LSEN_MASK : 0x00) | USB_ADDR_ADDR(_addr);
}

void USBHALHost::setEndpoint() {
  uint8_t val;
    USB0->ENDPOINT[0].ENDPT = val = (root_lowSpeed ? USB_ENDPT_HOSTWOHUB_MASK : 0x00)|
                              USB_ENDPT_RETRYDIS_MASK|
                              USB_ENDPT_EPCTLDIS_MASK|
                              USB_ENDPT_EPRXEN_MASK|
                              USB_ENDPT_EPTXEN_MASK|
                              USB_ENDPT_EPHSHK_MASK;
    printf("setEndpoint[%d] (0x%08x) = %02x\n", 0, &USB0->ENDPOINT[0].ENDPT, val); 
}

void USBHALHost::token_transfer_init() {
    tx_ptr = ODD;
    rx_ptr = ODD;
}

int USBHALHost::token_setup(USBEndpoint* ep, SETUP_PACKET* setup, uint16_t wLength) {
    USBDeviceConnected* dev = ep->getDevice();
    setAddr(dev->getAddress(), dev->getSpeed());

    for(int retry = 0;; retry++) {
        token_ready();
        USB0->ENDPOINT[0].ENDPT = (root_lowSpeed ? USB_ENDPT_HOSTWOHUB_MASK : 0x00) |
                                  USB_ENDPT_RETRYDIS_MASK|
                                  USB_ENDPT_EPRXEN_MASK|
                                  USB_ENDPT_EPTXEN_MASK|
                                  USB_ENDPT_EPHSHK_MASK;
        CTASSERT(sizeof(SETUP_PACKET) == 8);
        setup->wLength = wLength;
        int idx = EP0_BDT_IDX(TX, tx_ptr);
        bdt[idx].setBuffer((uint8_t*)setup, sizeof(SETUP_PACKET));
        bdt[idx].info = BD_OWN_MASK |
                        BD_DTS_MASK; // always data0
        token_done = false;
        USB0->TOKEN = USB_TOKEN_TOKENPID(SETUP_TOKEN)|
                      USB_TOKEN_TOKENENDPT(ep->getAddress() & 0x7f);
        while(!token_done);
        LastStatus = bdt[idx].getStatus();
        if (LastStatus == ACK) {
            if (retry > 0) {
                USB_DBG("retry=%d %02x", retry, prev_LastStatus);
            }
            break;
        } else if (LastStatus == STALL) {
            report.stall++;
            return STALL;
        }
        if (retry > 10) {
            USB_DBG("retry=%d", retry);
            break;
        }
        prev_LastStatus = LastStatus;
        wait_ms(100 * retry);
    }
    ep->setData01(DATA1); // next toggle
    return LastStatus;
}

int USBHALHost::token_in(USBEndpoint* ep, uint8_t* data, int size, int retryLimit) {
    USBDeviceConnected* dev = ep->getDevice();
    setAddr(dev->getAddress(), dev->getSpeed());
    setEndpoint();

    for(int retry = 0;; retry++) {
        token_ready();
        int idx = EP0_BDT_IDX(RX, rx_ptr);
        bdt[idx].setBuffer(data, size);
        bdt[idx].info = BD_OWN_MASK|
                        BD_DTS_MASK|
                        (ep->getData01() == DATA1 ? BD_DATA01_MASK : 0);
        token_done = false;
        USB0->TOKEN = USB_TOKEN_TOKENPID(IN_TOKEN)|
                      USB_TOKEN_TOKENENDPT(ep->getAddress()&0x7f);
        while(!token_done);
        LastStatus = bdt[idx].getStatus();
        int len = bdt[idx].byte_count;
        if (LastStatus == DATA0 || LastStatus == DATA1) {
            USB_TEST_ASSERT(ep->getData01() == LastStatus);
            ep->setData01(LastStatus == DATA0 ? DATA1 : DATA0);
            if (retry > 0) {
                //USB_DBG("len=%d retry=%d %02x", len, retry, prev_LastStatus);
            }
            return len;
        } else if (LastStatus == STALL) {
            report.stall++;
            return -1;
        } else if (LastStatus == NAK) {
            report.nak++;
            if (retry >= retryLimit) {
                if (retryLimit > 0) {
                    USB_DBG("retry=%d retryLimit=%d", retry, retryLimit);
                }
                return -1;
            }
            wait_ms(100 * retry);
        } else if (LastStatus == Bus_Timeout) {
            if (retry >= retryLimit) {
                if (retryLimit > 0) {
                    USB_DBG("Bus_Timeout retry=%d retryLimit=%d", retry, retryLimit);
                }                
                return -1;
            }
            wait_ms(500 + 100 * retry);
        } else {
            return -1;
        }
        prev_LastStatus = LastStatus;
    }
}

int USBHALHost::token_out(USBEndpoint* ep, const uint8_t* data, int size, int retryLimit) {
    USBDeviceConnected* dev = ep->getDevice();
    USB_TEST_ASSERT(dev);
    setAddr(dev->getAddress(), dev->getSpeed());
    setEndpoint();

    for(int retry = 0;; retry++) {
        token_ready();
        int idx = EP0_BDT_IDX(TX, tx_ptr);
        bdt[idx].setBuffer((uint8_t*)data, size);
        bdt[idx].info = BD_OWN_MASK|
                        BD_DTS_MASK|
                       (ep->getData01() == DATA1 ? BD_DATA01_MASK : 0);
        token_done = false;
	uint8_t val;
        USB0->TOKEN = val = USB_TOKEN_TOKENPID(OUT_TOKEN)|
                      USB_TOKEN_TOKENENDPT(ep->getAddress()&0x7f);
	USB_DBG("Set TOKEN = 0x%02x\n", val);
        while(!token_done);
        LastStatus = bdt[idx].getStatus();
        int len = bdt[idx].byte_count;
        USB_DBG("len=%d %02x", len, LastStatus);
        if (LastStatus == ACK) {
            ep->toggleData01();
            if (retry > 0) {
                USB_DBG("len=%d retry=%d %02x", len, retry, prev_LastStatus);
            }
            return len;
        } else if (LastStatus == STALL) {
            report.stall++;
            return -1;
        } else if (LastStatus == NAK) {
            report.nak++;
            if (retry > retryLimit) {
                USB_DBG("retry=%d retryLimit=%d", retry, retryLimit);
                return -1;
            }
            wait_ms(100 * retry);
        } else {
            return -1;
        }
        prev_LastStatus = LastStatus;
    }
}

void USBHALHost::multi_token_inNB(USBEndpoint* ep, uint8_t* data, int size) {
    USB_TRACE1(size);
    USB_TEST_ASSERT(ep->getState() != USB_TYPE_PROCESSING);
    ep->setBuffer(data, size);
    ep->setState(USB_TYPE_PROCESSING);
}

USB_TYPE USBHALHost::multi_token_inNB_result(USBEndpoint* ep) {
    USB_TEST_ASSERT(ep->getState() == USB_TYPE_PROCESSING);
    uint8_t* buf = ep->getBufStart();
    int size = ep->getBufSize();
    int result = multi_token_in(ep, buf, size, false);
    USB_TRACE1(result);
    if (result < 0) {
        return USB_TYPE_PROCESSING;
    }
    ep->setLengthTransferred(result);
    ep->setState(USB_TYPE_IDLE);
    return USB_TYPE_OK;
}

void USBHALHost::setToggle(USBEndpoint* ep, uint8_t toggle) {
    USB_TEST_ASSERT(toggle == 1);
    ep->setData01(toggle == 0 ? DATA0 : DATA1);
}

int USBHALHost::token_iso_in(USBEndpoint* ep, uint8_t* data, int size) {
    USBDeviceConnected* dev = ep->getDevice();
    USB_TEST_ASSERT(dev);
    setAddr(dev->getAddress());

    while(USB0->CTL & USB_CTL_TXSUSPENDTOKENBUSY_MASK); // TOKEN_BUSY ?
    USB0->ISTAT |= USB_ISTAT_SOFTOK_MASK; // Clear SOF
    while (!(USB0->ISTAT & USB_ISTAT_SOFTOK_MASK));
    USB0->SOFTHLD = 0; // this is needed as without this you can get errors
    USB0->ISTAT |= USB_ISTAT_SOFTOK_MASK; // clear SOF

    USB0->ENDPOINT[0].ENDPT = USB_ENDPT_EPCTLDIS_MASK|
                              USB_ENDPT_RETRYDIS_MASK|
                              USB_ENDPT_EPRXEN_MASK|
                              USB_ENDPT_EPTXEN_MASK;
    int idx = EP0_BDT_IDX(RX, rx_ptr);
    bdt[idx].setBuffer(data, size);
    bdt[idx].info = BD_OWN_MASK|
                    BD_DTS_MASK; // always DATA0
    token_done = false;
    USB0->TOKEN = USB_TOKEN_TOKENPID(IN_TOKEN)|
                  USB_TOKEN_TOKENENDPT(ep->getAddress()&0x7f);
    while(!token_done);
    LastStatus = bdt[idx].getStatus();
    int len = bdt[idx].byte_count;
    if (LastStatus == DATA0) {
        return len;
    }
    return -1;
}

void USBHALHost::token_ready() {
    while(USB0->CTL & USB_CTL_TXSUSPENDTOKENBUSY_MASK) { // TOKEN_BUSY ?
        wait_ms(1);
    }
    USB0->ISTAT |= USB_ISTAT_SOFTOK_MASK; // Clear SOF
    while (!(USB0->ISTAT & USB_ISTAT_SOFTOK_MASK));
    USB0->SOFTHLD = 0; // this is needed as without this you can get errors
    USB0->ISTAT |= USB_ISTAT_SOFTOK_MASK; // clear SOF
}

int USBHALHost::multi_token_in(USBEndpoint* ep, uint8_t* data, size_t total, bool block) {
    if (total == 0) {
        return token_in(ep);
    }
    int retryLimit = block ? 10 : 0;
    int read_len = 0;
    for(int n = 0; read_len < total; n++) {
        int size = std::min((int)total-read_len, ep->getSize());
        int result = token_in(ep, data+read_len, size, retryLimit);
        if (result < 0) {
            if (block) {
                return -1;
            }
            if (LastStatus == NAK) {
                if (n == 0) {
                    return -1;
                }
                break;
            }
            return result;
        }
        read_len += result;
        if (result < ep->getSize()) {
            break;
        }
    }
    return read_len;
}

int USBHALHost::multi_token_out(USBEndpoint* ep, const uint8_t* data, size_t total) {
    if (total == 0) {
        return token_out(ep);
    }
    int write_len = 0;
    for(int n = 0; write_len < total; n++) {
        int size = std::min((int)total-write_len, ep->getSize());
	USB_DBG("token_out sending %d of %d limit %d", size, total, ep->getSize());
        int result = token_out(ep, data+write_len, size);
        if (result < 0) {
            if (LastStatus == NAK) {
                if (n == 0) {
                    return -1;
                }
                break;
            }
            USB_DBG("token_out result=%d %02x", result, LastStatus);
            return result;
        }
        write_len += result;
        if (result < ep->getSize()) {
            break;
        }
    }
    return write_len;
}

void USBHALHost::_usbisr(void) {
    if (instHost) {
        instHost->UsbIrqhandler();
    }
}

void USBHALHost::UsbIrqhandler() {
    if (USB0->ISTAT & USB_ISTAT_TOKDNE_MASK) {
        uint8_t stat = USB0->STAT;
        ODD_EVEN next_ptr = (stat & USB_STAT_ODD_MASK) ? ODD : EVEN;
        if (stat & USB_STAT_TX_MASK) {
            tx_ptr = next_ptr;
        } else {
            rx_ptr = next_ptr;
        }
        USB0->ISTAT = USB_ISTAT_TOKDNE_MASK;
        token_done = true;
    }
    if (USB0->ISTAT & USB_ISTAT_ATTACH_MASK) {
        USB0->INTEN &= ~USB_INTEN_ATTACHEN_MASK;
        USB0->ISTAT = USB_ISTAT_ATTACH_MASK;
        attach_done = true;
    }
    if (USB0->ISTAT & USB_ISTAT_ERROR_MASK) {
        uint8_t errstat = USB0->ERRSTAT;
        if (errstat & USB_ERRSTAT_PIDERR_MASK) {
            report.errstat_piderr++;
        }
        if (errstat & USB_ERRSTAT_CRC5EOF_MASK) {
            report.errstat_crc5eof++;
        }
        if (errstat & USB_ERRSTAT_CRC16_MASK) {
            report.errstat_crc16++;
        }
        if (errstat & USB_ERRSTAT_DFN8_MASK) {
            report.errstat_dfn8++;
        }
        if (errstat & USB_ERRSTAT_BTOERR_MASK) {
            report.errstat_btoerr++;
        }
        if (errstat & USB_ERRSTAT_DMAERR_MASK) {
            report.errstat_dmaerr++;
        }
        if (errstat & USB_ERRSTAT_BTSERR_MASK) {
            report.errstat_btoerr++;
        }
        USB0->ERRSTAT = errstat;
        USB0->ISTAT = USB_ISTAT_ERROR_MASK;
    }
}

void Report::clear() {
    errstat_piderr = 0;
    errstat_crc5eof = 0;
    errstat_crc16 = 0;
    errstat_dfn8 = 0;
    errstat_btoerr = 0;
    errstat_dmaerr = 0;
    errstat_bsterr = 0;
    //
    nak = 0;
}

void Report::print_errstat() {
    printf("ERRSTAT PID: %d, CRC5EOF: %d, CRC16: %d, DFN8: %d, BTO: %d, DMA: %d, BST: %d\n",
        errstat_piderr, errstat_crc5eof,
        errstat_crc16, errstat_dfn8,
        errstat_btoerr, errstat_dmaerr, errstat_bsterr);
}

void debug_hex(uint8_t* buf, int size) {
    int n = 0;
    for(int i = 0; i < size; i++) {
        fprintf(stderr, "%02x ", buf[i]);
        if (++n >= 16) {
            fprintf(stderr, "\n");
            n = 0;
        }
    }
    if (n > 0) {
        fprintf(stderr, "\n");
    }
}

#endif


