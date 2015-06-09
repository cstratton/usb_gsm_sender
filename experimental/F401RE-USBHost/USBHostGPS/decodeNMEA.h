// decodeNMEA.h 2012/12/15
#ifndef DECODE_NMEA_H
#define DECODE_NMEA_H

class decodeNMEA {
public:
    decodeNMEA();
    void inputNMEA(char* buf, int len);
    void inputNMEA(char c);
    float lat,lon;
    time_t gprmc_t;
    time_t update_t;

private:
    void parse(int type, int row, char* buf);
    void update(int type, int row);
    int m_seq;
    int m_type;
    int m_row;
    uint8_t m_sum;
    char m_buf[12];
    int m_buf_pos;
    bool m_status;
    float tmp_lat,tmp_lon;
    struct tm tmp_timeinfo;
};

#endif // DECODE_NMEA_H
