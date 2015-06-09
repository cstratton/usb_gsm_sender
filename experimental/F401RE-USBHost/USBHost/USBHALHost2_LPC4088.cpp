#if defined(TARGET_LPC4088)||defined(TARGET_LPC1768)
#include "USBHALHost.h"

#undef USB_TEST_ASSERT
void usb_test_assert_internal(const char *expr, const char *file, int line);
#define USB_TEST_ASSERT(EXPR) while(!(EXPR)){usb_test_assert_internal(#EXPR,__FILE__,__LINE__);}

#define USB_INFO(...) do{fprintf(stderr,__VA_ARGS__);fprintf(stderr,"\n");}while(0);

//#define DBG_USE_POSIX_MEMALIGN

#ifdef DBG_USE_POSIX_MEMALIGN
void* usb_ram_malloc(size_t size, int aligment)
{
    TEST_ASSERT(aligment >= 4);
    TEST_ASSERT(!(aligment & 3));
    void* p;
    if (posix_memalign(&p, aligment, size) == 0) {
        return p;
    }
    return NULL;
}

void usb_ram_free(void* p)
{
    free(p);
}
#else

#define CHUNK_SIZE 64

#if defined(TARGET_LPC1768)
#define USB_RAM_BASE 0x2007C000
#define USB_RAM_SIZE 0x4000
#define BLOCK_COUNT (USB_RAM_SIZE/CHUNK_SIZE)
#elif defined(TARGET_LPC4088)
#define USB_RAM_BASE 0x20000000
#define USB_RAM_SIZE 0x4000
#define BLOCK_COUNT (USB_RAM_SIZE/CHUNK_SIZE)
#else
#error "target error"
#endif

static uint8_t* ram = NULL;
static uint8_t* map;

static void usb_ram_init()
{
    USB_INFO("USB_RAM: 0x%p(%d)", USB_RAM_BASE, USB_RAM_SIZE);
    ram = (uint8_t*)USB_RAM_BASE;
    USB_TEST_ASSERT((int)ram%256 == 0);
    map = (uint8_t*)malloc(BLOCK_COUNT);
    USB_TEST_ASSERT(map);
    memset(map, 0, BLOCK_COUNT);
}

// first fit malloc
void* usb_ram_malloc(size_t size, int aligment)
{
    USB_TEST_ASSERT(aligment >= 4);
    USB_TEST_ASSERT(!(aligment & 3));
    if (ram == NULL) {
        usb_ram_init();
    }
    int needs = (size+CHUNK_SIZE-1)/CHUNK_SIZE;
    void* p = NULL;
    for(int idx = 0; idx < BLOCK_COUNT;) {
        bool found = true;
        for(int i = 0; i < needs; i++) {
            int block = map[idx + i];
            if (block != 0) {
                idx += block;
                found = false;
                break;
            }
        }
        if (!found) {
            continue;
        }
        p = ram+idx*CHUNK_SIZE;
        if ((int)p % aligment) {
            idx++;
            continue;
        }
        USB_TEST_ASSERT((idx + needs) <= BLOCK_COUNT);
        for(int i = 0; i < needs; i++) {
            map[idx + i] = needs - i;
        }
        break;
    }
    USB_TEST_ASSERT(p);
    return p;
}

void usb_ram_free(void* p)
{
    USB_TEST_ASSERT(p >= ram);
    USB_TEST_ASSERT(p < (ram+CHUNK_SIZE*BLOCK_COUNT));
    int idx = ((int)p-(int)ram)/CHUNK_SIZE;
    int block = map[idx];
    USB_TEST_ASSERT(block >= 1);
    for(int i =0; i < block; i++) {
        map[idx + i] = 0;
    }
}

#endif // DBG_USE_POSIX_MEMALIGN

void print_td(FILE* stream, HCTD* td)
{
    if (td == NULL) {
        fprintf(stream, "TD %p:\n", td);
        return;
    }
    uint32_t* p = reinterpret_cast<uint32_t*>(td);
    fprintf(stream, "TD %p: %08X %08X %08X %08X", p, p[0], p[1], p[2], p[3]);
    fprintf(stream, " ep=%p\n", td->parent);
    uint8_t* bp = reinterpret_cast<uint8_t*>(p[1]);
    uint8_t* be = reinterpret_cast<uint8_t*>(p[3]);
    if (bp) {
        fprintf(stream, "BF %p:", bp);
        while(bp <= be) {
            fprintf(stream, " %02X", *bp);
            bp++;
        }
        fprintf(stream, "\n");
    } 
}

void print_ed(FILE* stream, HCED* ed)
{
    uint32_t* p = reinterpret_cast<uint32_t*>(ed);
    while(p) {
        fprintf(stream, "ED %p: %08X %08X %08X %08X\n", p, p[0], p[1], p[2], p[3]);
        HCTD* td = reinterpret_cast<HCTD*>(p[2] & ~3);
        HCTD* tdtail = reinterpret_cast<HCTD*>(p[1]);
        while(td != NULL && td != tdtail) {
            print_td(stream, td);
            td = td->Next;
        }
        p = reinterpret_cast<uint32_t*>(p[3]);
    }
}

void print_itd(FILE* stream, HCITD* itd)
{
    if (itd == NULL) {
        fprintf(stream, "ITD %p:\n", itd);
        return;
    }
    uint32_t* p = reinterpret_cast<uint32_t*>(itd);
    fprintf(stream, "ITD %p: %08X %08X %08X %08X", p, p[0], p[1], p[2], p[3]);
    fprintf(stream, " ep=%p\n", itd->parent);
    uint16_t* offset = reinterpret_cast<uint16_t*>(p+4);
    fprintf(stream, "ITD %p: %04X %04X %04X %04X %04X %04X %04X %04X\n", offset, 
        offset[0], offset[1], offset[2], offset[3], offset[4], offset[5], offset[6], offset[7]);
}

void print_ied(FILE* stream, HCED* ed)
{
    uint32_t* p = reinterpret_cast<uint32_t*>(ed);
    while(p) {
        fprintf(stream, "ED %p: %08X %08X %08X %08X\n", p, p[0], p[1], p[2], p[3]);
        HCITD* itd = reinterpret_cast<HCITD*>(p[2] & ~3);
        HCITD* itdtail = reinterpret_cast<HCITD*>(p[1]);
        while(itd != NULL && itd != itdtail) {
            print_itd(stream, itd);
            itd = itd->Next;
        }
        p = reinterpret_cast<uint32_t*>(p[3]);
    }
}

void print_bytes(FILE* stream, char* s, uint8_t* buf, int len)
{
    fprintf(stream, "%s %d:", s, len);
    for(int i = 0; i < len; i++) {
        fprintf(stream, " %02X", buf[i]);
    }
    fprintf(stream, "\n");
}

void print_hex(FILE* stream, uint8_t* p, int len)
{
    for(int i = 0; i < len; i++) {
        if (i%16 == 0) {
            fprintf(stream, "%p:", p);
        }
        fprintf(stream, " %02X", *p);
        p++;
        if (i%16 == 15) {
            fprintf(stream, "\n");
        }
    }
    fprintf(stream, "\n");
}

void assert_print(const char* pf, int line, const char* msg) {
    printf("\n\n%s@%d %s ASSERT!\n\n", pf, line, msg);
    exit(1);
}

#endif // defined(TARGET_LPC4088)||defined(TARGET_LPC1768)


