// Simple USBHost Mouse for FRDM-KL46Z test program

//#include "USBHostMouse.h"
//#include "USBHostKeyboard.h"
#include "USBHostE303.h"

//DigitalOut led1(LED_GREEN);
//DigitalOut led2(LED_RED);
#define LED_OFF 1
#define LED_ON  0

void callback(uint8_t key) {
  //    led1 = (key&1) ? LED_ON : LED_OFF; // button on/off
  //  led2 = (key&2) ? LED_ON : LED_OFF;
    printf("key %02x\n", key);
}

void zero(uint8_t *buf, int len) {
  int i;
  for (i = 0; i<len; i++) 
    buf[i] = 0;
}

Serial pc(USBTX, USBRX);
/*
ssize_t _write(int fd, void *buf, ssize_t count)  {
  int i;
  for (i=0 ; i<count; i++)
    pc.putc(((char *)buf)[i]);
}

int puts(const char *s) {
  return pc.puts(s);
}

int fputs(const char *s, FILE *stream) {
  return puts(s);
}

int fputc(int c, FILE *stream) {
  return pc.putc(c);
}

int putc(int c, FILE *stream) {
 return pc.putc(c);
}

int putchar(int c) {
  return pc.putc(c);
}
*/


//LocalFileSystem local("local");
int main() {
    // printf to specific peripherals
	pc.printf("st-flash still works\n");
    pc.printf("Hello World!\n");
    // printf to stdout
    printf("Hello USB Serial World!\n");
    printf("hello with an argument %d\n", SystemCoreClock);
    printf("and again without one\n");
    // change stdout to pc
    //  freopen("/pc", "w", stdout);
    printf("Hello external World!\n");


    pc.printf("core clock=%d\n", SystemCoreClock);;
    printf("but this goes nowhere\n");

    printf("Looking for pre-switch E303...\n");
    USBHostE303PreSwitch pre;
    int connected = 0;
    do {     
      connected = pre.connect();
      if (!connected) error("USB mouse not found.\n");
    } while (!connected);
    printf("connected (main)!\n");
    wait_ms(100);
    while (1) {
    {    
      printf("trying to set USSD mode\r\n");
      char *cmd = "AT^USSDMODE=0\r";
      uint8_t response[64];
      zero(response,64);
      int res = pre.bulk_write(cmd, strlen(cmd));
      printf("writing returned %d\n", res);
           USBHost::poll();
      wait_ms(100);
           USBHost::poll();
      printf("trying to read....\n");
      res = pre.bulk_read(response, 32);
      printf("read response returns %d %s\n", res, res ? (unsigned char *) "invalid" : response);
      int i;
      for (i = 0; i < 32; i++) {
	printf(" %02x", response[i]);
	if ((i&7)==7) printf("\n");
      }
    }


    {    
      printf("trying to send USSD");
      char *cmd = "AT+CUSD=1,\"HIFROMHANDMADEKL25Z64Board\",15\r";
      uint8_t response[64];
      zero(response,64);
      int res = pre.bulk_write(cmd, strlen(cmd));
      printf("writing returned %d\n", res);
      USBHost::poll();
      wait_ms(100); 
      USBHost::poll();
     printf("trying to read....\n");
 
      res = pre.bulk_read(response, 32);
      printf("read response returns %d %s\n", res, res ? (unsigned char *) "invalid" : response);
      int i;
      for (i = 0; i < 32; i++) {
	printf(" %02x", response[i]);
	if ((i&7)==7) printf("\n");
      }
    }

    int readcount = 0;
    for (readcount = 0; readcount < 5; readcount++) {
           USBHost::poll();
      uint8_t response[64];
      zero(response,64);
      int res = pre.bulk_read(response, 32);
      if (!res) 
	printf("\r\nread response returns %d %s\n", res, res ? (unsigned char *) "invalid" : response);
      else {
	printf(".");
	fflush(stdout);
      }
    }
    printf("\r\nLooping again \r\n");
      }

    while(1) {
           USBHost::poll();
    }
}

