// CamInfo.cpp
#include "USBHostCam.h"

// Logitech C270
#define C270_VID 0x046d
#define C270_PID 0x0825
#define C270_160x120 2
#define C270_176x144 3
#define C270_320x176 4
#define C270_320x240 5
#define C270_352x288 6
#define C270_432x240 7
#define C270_640x480 1
#define C270_544x288 8
#define C270_640x360 9
#define C270_752x416 10
#define C270_800x448 11
#define C270_800x600 12

#define C270_MJPEG 2
#define C270_YUV2  1

#define C270_EN  0x81
#define C270_MPS  192
#define C270_IF_ALT_192 1
#define C270_IF_ALT(A) C270_IF_ALT_##A

#define C270_INFO(SIZE) {C270_VID, C270_PID, _##SIZE, 0, \
    "C270", \
    C270_MJPEG, \
    C270_##SIZE, \
    _5FPS, \
    C270_EN, \
    192, \
    C270_IF_ALT(192), \
    4, \
    3}

#define C210_PID 0x819
#define C210_INFO(SIZE) {C270_VID, C210_PID, _##SIZE, 0, \
    "C270", \
    C270_MJPEG, \
    C270_##SIZE, \
    _5FPS, \
    C270_EN, \
    192, \
    C270_IF_ALT(192), \
    4, \
    3}

// Logitech Qcam Orbit AF QCAM-200R
#define Q200R_VID 0x046d
#define Q200R_PID 0x0994
#define Q200R_160x120 1
#define Q200R_176x144 2
#define Q200R_320x240 3
#define Q200R_352x288 4
#define Q200R_640x480 5
#define Q200R_800x600 6

#define Q200R_MJPEG 1
#define Q200R_YUV2  2

#define Q200R_EN  0x81
#define Q200R_MPS  192
#define Q200R_IF_ALT_192 1
#define Q200R_IF_ALT_384 2
#define Q200R_IF_ALT_512 3
#define Q200R_IF_ALT_640 4
#define Q200R_IF_ALT_800 5
#define Q200R_IF_ALT_944 6
#define Q200R_IF_ALT(A) Q200R_IF_ALT_##A
#define Q200R_INFO(SIZE) {Q200R_VID, Q200R_PID, _##SIZE, 0, \
    "Q200R", \
    Q200R_MJPEG, \
    Q200R_##SIZE, \
    _5FPS, \
    Q200R_EN, \
    192, \
    Q200R_IF_ALT(192), \
    4, \
    3}

//LifeCam VX700 / VX500
#define VX700_VID 0x045e
#define VX700_PID 0x074a

#define VX700_160x120 5
#define VX700_176x144 4
#define VX700_320x240 3
#define VX700_352x288 2
#define VX700_640x480 1

#define VX700_MJPEG 1

#define VX700_EN  0x81
#define VX700_MPS  128
#define VX700_IF_ALT_128 1 
#define VX700_IF_ALT(A) VX700_IF_ALT_##A
#define VX700_INFO(SIZE) {VX700_VID, VX700_PID, _##SIZE, 0, \
    "VX700", \
    VX700_MJPEG, \
    VX700_##SIZE, \
    _5FPS, \
    VX700_EN, \
    128, \
    VX700_IF_ALT(128), \
    4, \
    3}

//Sonix USB 2.0 Camera
#define SONIX_160x120 5
#define SONIX_176x144 4
#define SONIX_320x240 3
#define SONIX_352x288 2
#define SONIX_640x480 1

#define SONIX_IF_ALT_128 1
#define SONIX_IF_ALT_256 2
#define SONIX_IF_ALT_512 3
#define SONIX_IF_ALT_600 4
#define SONIX_IF_ALT_800 5
#define SONIX_IF_ALT_956 6
#define SONIX_IF_ALT(A) SONIX_IF_ALT_##A
#define SONIX_INFO(SIZE) {0x0c45, 0x62c0, _##SIZE, 0, \
    "SONIX", \
    1, \
    SONIX_##SIZE, \
    _5FPS, \
    0x81, \
    128, \
    SONIX_IF_ALT(128), \
    4, \
    3}

static const CamInfo CamInfoList[] = {
// Logitech C270
C270_INFO(160x120),
C270_INFO(176x144),
C270_INFO(320x176),
C270_INFO(320x240),
C270_INFO(352x288),
C270_INFO(432x240),
C270_INFO(640x480),
C270_INFO(544x288),
C270_INFO(640x360),
C270_INFO(752x416),
C270_INFO(800x448),
C270_INFO(800x600),

// Logitech C210
C210_INFO(160x120),
C210_INFO(176x144),
C210_INFO(320x176),
C210_INFO(320x240),
C210_INFO(352x288),
C210_INFO(432x240),
C210_INFO(640x480),
C210_INFO(544x288),
C210_INFO(640x360),
C210_INFO(752x416),
C210_INFO(800x448),
C210_INFO(800x600),

// Logitech Qcam Orbit AF QCAM-200R
Q200R_INFO(160x120),
Q200R_INFO(176x144),
Q200R_INFO(320x240),
Q200R_INFO(352x288),
Q200R_INFO(640x480),
Q200R_INFO(800x600),

// LifeCam VX700
VX700_INFO(160x120),
VX700_INFO(176x144),
VX700_INFO(320x240),
VX700_INFO(352x288),
VX700_INFO(640x480),

// Sonix USB 2.0 Camera
SONIX_INFO(160x120),
SONIX_INFO(176x144),
SONIX_INFO(320x240),
SONIX_INFO(352x288),
SONIX_INFO(640x480),

// Not found
{0,0,0,0},
};

CamInfo* getCamInfoList() {
    return const_cast<CamInfo*>(CamInfoList);
}
