// USBHostCam.h
#include "USBHostConf.h"
#include "USBHost.h"
#include "BaseUvc.h"
#include "decodeMJPEG.h"
#pragma once

#define _160x120 2
#define _176x144 3
#define _320x176 4
#define _320x240 5
#define _352x288 6
#define _432x240 7
#define _640x480 1
#define _544x288 8
#define _640x360 9
#define _752x416 10
#define _800x448 11
#define _800x600 12

#define TEST_ASSERT(A) while(!(A)){fprintf(stderr,"\n\n%s@%d %s ASSERT!\n\n",__PRETTY_FUNCTION__,__LINE__,#A);exit(1);};

struct CamInfo {
    uint16_t vid;
    uint16_t pid;
    uint8_t size;
    uint8_t option;
//
    const char* name;
    uint8_t formatIndex;
    uint8_t frameIndex;
    uint32_t interval;
    uint8_t en;
    uint8_t mps;
    uint8_t if_alt;
    uint8_t frameCount; // ITD frame count 1-8
    uint8_t queueLimit; // ITD queue limit 1-3
};

/** 
 * A class to communicate a Cam
 */
class USBHostCam : public IUSBEnumerator, public BaseUvc, public decodeMJPEG {
public:
    /**
    * Constructor
    *
    */
    USBHostCam(uint8_t size = _160x120, uint8_t option = 0, CamInfo* user_caminfo = NULL);

    /**
    * Check if a Cam device is connected
    *
    * @return true if a Cam device is connected
    */
    bool connected();
 
    /**
     * Try to connect to a Cam device
     *
     * @return true if connection was successful
     */
    bool connect();

    /**
     * read jpeg image
     *
     * @param buf read buffer 
     * @param size buffer size 
     * @param timeout_ms timeout default 15sec
     * @return jpeg size if read success else -1
     */
    int readJPEG(uint8_t* buf, int size, int timeout_ms = 15*1000);

protected:
    //From IUSBEnumerator
    virtual void setVidPid(uint16_t vid, uint16_t pid);
    virtual bool parseInterface(uint8_t intf_nb, uint8_t intf_class, uint8_t intf_subclass, uint8_t intf_protocol); //Must return true if the interface should be parsed
    virtual bool useEndpoint(uint8_t intf_nb, ENDPOINT_TYPE type, ENDPOINT_DIRECTION dir); //Must return true if the endpoint will be used

private:
    bool dev_connected;
 
    int cam_intf;
    bool device_found;
    bool caminfo_found;

    uint8_t _seq;
    uint8_t* _buf;
    int _pos;
    int _size;
    CamInfo* CamInfoList;
    CamInfo* caminfo;
    uint8_t _caminfo_size;
    uint8_t _caminfo_option;

    virtual void outputJPEG(uint8_t c, int status); // from decodeMJPEG
    void callback_motion_jpeg(uint16_t frame, uint8_t* buf, int len);
    void init();
};
