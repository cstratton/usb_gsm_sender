// USBHostCam.cpp
#include "USBHostCam.h"

#if 0
#define CAM_DBG(x, ...) std::printf("[%s:%d]"x"\n", __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__);
#else
#define CAM_DBG(...)  while(0);
#endif
#define CAM_INFO(...) do{fprintf(stderr,__VA_ARGS__);}while(0);

CamInfo* getCamInfoList(); // CamInfo.cpp

USBHostCam::USBHostCam(uint8_t size, uint8_t option, CamInfo* user_caminfo)
{
    CAM_DBG("size: %d, option: %d", size, option);
    _caminfo_size = size;
    _caminfo_option = option;
    if (user_caminfo) {
        CamInfoList = user_caminfo;
    } else {
        CamInfoList = getCamInfoList();
    }
    clearOnResult();
    host = USBHost::getHostInst();
    init();
}

void USBHostCam::init()
{
    CAM_DBG("");
    dev_connected = false;
    dev = NULL;
    ep_iso_in = NULL;
    cam_intf = -1;
    device_found = false;
    caminfo_found = false;
}
 
bool USBHostCam::connected()
{
    return dev_connected;
}
 
bool USBHostCam::connect()
{
    if (dev_connected) {
        return true;
    }
 
    for (uint8_t i = 0; i < MAX_DEVICE_CONNECTED; i++) {
        if ((dev = host->getDevice(i)) != NULL) {
            
            CAM_DBG("Trying to connect Cam device\r\n");
            
            if(host->enumerate(dev, this)) {
                break;
            }
            if (device_found) {
                USB_INFO("New Cam: %s device: VID:%04x PID:%04x [dev: %p - intf: %d]", caminfo->name, dev->getVid(), dev->getPid(), dev, cam_intf);
                ep_iso_in = new USBEndpoint(dev); 
                ep_iso_in->init(ISOCHRONOUS_ENDPOINT, IN, caminfo->mps, caminfo->en);
                ep_iso_in->ohci_init(caminfo->frameCount, caminfo->queueLimit);
                uint8_t buf[26];
                memset(buf, 0, sizeof(buf));
                buf[2] = caminfo->formatIndex;
                buf[3] = caminfo->frameIndex;
                *reinterpret_cast<uint32_t*>(buf+4) = caminfo->interval;
                USB_TYPE res = Control(SET_CUR, VS_COMMIT_CONTROL, 1, buf, sizeof(buf));
                if (res != USB_TYPE_OK) {
                    USB_ERR("SET_CUR VS_COMMIT_CONTROL FAILED");
                }
                res = setInterfaceAlternate(1, caminfo->if_alt);
                if (res != USB_TYPE_OK) {
                    USB_ERR("SET_INTERFACE FAILED");
                }
                dev_connected = true;
                return true;
            }
        }
    }
    init();
    return false;
}

/*virtual*/ void USBHostCam::setVidPid(uint16_t vid, uint16_t pid)
{
    CAM_DBG("vid:%04x,pid:%04x", vid, pid);
    caminfo = CamInfoList;
    while(caminfo->vid != 0) {
        if (caminfo->vid == vid && caminfo->pid == pid && 
            caminfo->size == _caminfo_size && caminfo->option == _caminfo_option) {
            caminfo_found = true;
            break;
        }
        caminfo++;
    }
}
 
/*virtual*/ bool USBHostCam::parseInterface(uint8_t intf_nb, uint8_t intf_class, uint8_t intf_subclass, uint8_t intf_protocol) //Must return true if the interface should be parsed
{
    CAM_DBG("intf_nb=%d,intf_class=%02X,intf_subclass=%d,intf_protocol=%d", intf_nb, intf_class, intf_subclass, intf_protocol);
    if ((cam_intf == -1) && caminfo_found) {
        cam_intf = intf_nb;
        device_found = true;
        return true;
    }
    return false;
}
 
/*virtual*/ bool USBHostCam::useEndpoint(uint8_t intf_nb, ENDPOINT_TYPE type, ENDPOINT_DIRECTION dir) //Must return true if the endpoint will be used
{
    CAM_DBG("intf_nb:%d,type:%d,dir:%d",intf_nb, type, dir);
    return false;
}

#define SEQ_READ_IDLE 0
#define SEQ_READ_EXEC 1
#define SEQ_READ_DONE 2

int USBHostCam::readJPEG(uint8_t* buf, int size, int timeout_ms) {
    _buf = buf;
    _pos = 0;
    _size = size;
    _seq = SEQ_READ_IDLE;
    setOnResult(this, &USBHostCam::callback_motion_jpeg);
    Timer timeout_t;
    timeout_t.reset();
    timeout_t.start();
    while(timeout_t.read_ms() < timeout_ms && _seq != SEQ_READ_DONE) {
        poll();
    } 
    return _pos;
}

/* virtual */ void USBHostCam::outputJPEG(uint8_t c, int status) { // from decodeMJPEG
    if (_seq == SEQ_READ_IDLE) {
        if (status == JPEG_START) {
            _pos = 0;
            _seq = SEQ_READ_EXEC;
        }
    }
    if (_seq == SEQ_READ_EXEC) {
        if (_pos < _size) {
            _buf[_pos++] = c;  
        }  
        if (status == JPEG_END) {
            _seq = SEQ_READ_DONE;
        }
    }
}

void USBHostCam::callback_motion_jpeg(uint16_t frame, uint8_t* buf, int len) {
        inputPacket(buf, len);
}
