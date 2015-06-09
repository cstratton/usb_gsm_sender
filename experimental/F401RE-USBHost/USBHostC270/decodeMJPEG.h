// decodeMJPEG.h 2012/12/9
#ifndef DECODE_MJPEG_H
#define DECODE_MJPEG_H

#define JPEG_NONE  0
#define JPEG_START 1
#define JPEG_END   2
#define JPEG_ERROR 3

class decodeMJPEG {
public:
    decodeMJPEG();
    void inputPacket(const uint8_t* buf, int len);
    virtual void outputJPEG(uint8_t c, int status = JPEG_NONE) = 0;
protected:
    void input(uint8_t c);
    int m_seq;
    uint8_t m_mark;
    uint16_t m_seg_pos; 
    uint16_t m_seg_len;
    bool m_bDHT;
    bool m_output_desable;
};

#endif // DECODE_MJPEG_H
