// decodeNMEA.cpp 2012/12/13
#include "mbed.h"
#include "decodeNMEA.h"

#define SEQ_INIT 0
#define SEQ_DATA 1
#define SEQ_SUM0 2
#define SEQ_SUM1 3

#define GPGGA 1
#define GPGLL 2
#define GPGSV 3
#define GPRMC 4
#define GPVTG 5
#define GPZDA 6

decodeNMEA::decodeNMEA():m_seq(0) {
    gprmc_t = 0;
    update_t = 0;
    m_status = false;
}

void decodeNMEA::inputNMEA(char* buf, int len) {
    for(int i = 0; i < len; i++) {
        inputNMEA(buf[i]);
    }
}

static int ctoh(char c) {
    if (c >= '0' && c <= '9') {
        return c-'0';
    }
    return c-'A'+10;
}
     
void decodeNMEA::inputNMEA(char c) {
    switch(m_seq) {
        case SEQ_INIT:
            if (c == '$') {
                m_type = 0;
                m_row = 0;
                m_buf_pos = 0;
                m_sum = 0x00;
                m_seq = SEQ_DATA;
            }
            break;
        case SEQ_DATA:
            m_sum ^= c;
            if (c == ',' || c == '*') {
                m_buf[m_buf_pos] = '\0';
                parse(m_type, m_row, m_buf);
                m_row++;
                m_buf_pos =0;
                if (c == '*') { // check sum ?
                    m_sum ^= c;
                    m_seq = SEQ_SUM0;
                }
            } else {
                if (m_buf_pos < sizeof(m_buf)-1) {
                    m_buf[m_buf_pos++] = c;
                }
            }
            break;
        case SEQ_SUM0:
            if (ctoh(c) == (m_sum>>4)) {
                m_seq = SEQ_SUM1;
            } else {
                m_seq = SEQ_INIT;
            }
            break;
        case SEQ_SUM1:
            if (ctoh(c) == (m_sum&0x0f)) {
                update(m_type, m_row);
            } else {
                
                m_seq = SEQ_INIT;
            }
            break;
        default:
            m_seq = SEQ_INIT;
            break;
    }
}

float DMMtoDegree(const char *s)
{
    char *p = strchr(const_cast<char*>(s), '.');
    if (p == NULL) {
        return 0.0;
    }
    const uint32_t k[] = {10000,1000,100,10,1};
    uint32_t i3 = atoi(p+1) * k[strlen(p+1)];
    uint32_t i2 = atoi(p-2);
    uint32_t i1 = atoi(s) / 100;

    uint32_t i = i1*10000*60 + (i2*10000 + i3);
    return i / 10000.0 / 60.0;
}

void decodeNMEA::parse(int type, int row, char* buf) {
    if (row == 0) {
        if (strcmp(buf, "GPRMC") == 0) {
            m_type = GPRMC;
            m_status = false;
        } else {
            m_type = 0;
        }
        return;
    }
    if (type == GPRMC) {
        switch(row) {
            case 1:
                tmp_timeinfo.tm_sec = atoi(buf+4);
                buf[4] = '\0';
                tmp_timeinfo.tm_min = atoi(buf+2);
                buf[2] = '\0';
                tmp_timeinfo.tm_hour = atoi(buf);
                break;
            case 2:
                if (buf[0] == 'A') {
                    m_status = true;
                }
                break;
            case 3:
                tmp_lat = DMMtoDegree(buf);
                break;
            case 4:
                if (buf[0] == 'S') {
                    tmp_lat *= -1;
                }
                break;
            case 5:
                tmp_lon = DMMtoDegree(buf);
                break;
            case 6:
                if (buf[0] == 'W') {
                    tmp_lon *= -1;
                }
                break;
            case 9:
                tmp_timeinfo.tm_year = 2000 - 1900 + atoi(buf+4);
                buf[4] = '\0';
                tmp_timeinfo.tm_mon = atoi(buf+2) - 1;
                buf[2] = '\0';
                tmp_timeinfo.tm_mday = atoi(buf);
                break;
        }
    }
}

void decodeNMEA::update(int type, int row) {
    if (type == GPRMC && m_status) {
        lat = tmp_lat;
        lon = tmp_lon;
        gprmc_t = mktime(&tmp_timeinfo);
        update_t = gprmc_t;
    }
}

