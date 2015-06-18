#include "USBHostE303.h"
//I'm sure there's a better way to do this
void zero(uint8_t *buf, int len) {
  int i;
  for (i = 0; i<len; i++) 
    buf[i] = 0;
}

//Serial pc(USBTX, USBRX); //is this still needed, or does implicit work?

int main() {
    printf("Hello external World!\n");
    printf("core clock=%lu\n", SystemCoreClock);;

    printf("Looking for e303-switch E303...\n");
    USBHostE303 e303;
    int connected = 0;
    do {     
      connected = e303.connect();
      if (!connected) error("E303 not found.\n");
    } while (!connected);
    printf("connected (main)!\n");
    wait_ms(100);
    while (1) {
    {    
      printf("trying to set USSD mode\r\n");
      const char *cmd = "AT^USSDMODE=0\r";
      uint8_t response[64];
      zero(response,64);
      int res = e303.bulk_write((void *)cmd, strlen(cmd));
      printf("writing returned %d\n", res);
           USBHost::poll();
      wait_ms(100);
           USBHost::poll();
      printf("trying to read....\n");
      res = e303.bulk_read(response, 32);
      printf("read response returns %d %s\n", res, 
	     (res < 0) ? (unsigned char *) "invalid" : response);
      int i;
      for (i = 0; i < 32; i++) {
	printf(" %02x", response[i]);
	if ((i&7)==7) printf("\n");
      }
    }
    

    {    
      printf("trying to send USSD");
      const char *cmd = "AT+CUSD=1,\"HIFROMHANDMADEKL25Z64Board\",15\r";
      uint8_t response[64]; //needs to be one more than maximum read - ie 33
      zero(response,64);
      int res = e303.bulk_write((void *)cmd, strlen(cmd));
      printf("writing returned %d\n", res);
      USBHost::poll();
      wait_ms(100); 
      USBHost::poll();
      printf("trying to read....\n");
      if (0) {
	res = e303.bulk_read(response, 32);
	printf("read response returns %d %s\n", res, 
	       (res < 0) ?  (unsigned char *) "invalid" : response);
	int i;
	if (i>0)
	  for (i = 0; i < res; i++) {
	    printf(" %02x", response[i]);
	    if ((i&7)==7) printf("\n");
	  }
      }
    }
    int readcount = 0;
    for (readcount = 0; readcount < 5; readcount++) {
           USBHost::poll();
	   uint8_t response[64];
	   zero(response,64);
	   int res = e303.bulk_read(response, 32);
	   if (res < 0) 
	     printf("\r\nread response returns %d invalid\n", res);
	   else if (res > 0) {
	     response[res]=0;
	     printf("\r\n read response returns %d : %s\n", res, response);
	     int i;
	     if (readcount == 0)
	       for (i = 0; i < res; i++) {
		 printf(" %02x", response[i]);
		 if ((i&7)==7) printf("\n");
	       }
	   } else {
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

