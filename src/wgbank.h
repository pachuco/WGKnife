#ifndef WGBANK_H
#define WGBANK_H

/* WinGroove soundbank format
 *
 * Reverse engineering by pachuco and Alex A. <coderain@sdf.org>
 *
 * Overall structure:
 * - file header
 * - 16 * 256 bytes of int8_t square and triangle waves, with no apparent use
 * - PatchMap, table of 256 * uint32_t, offsets to Patches
 * - another 256 * uint32_t, filler
 * - area of consecutive Patches, owning variable amount of Splits
 *     - if the Patch is a drumkit, we have a 128 byte DrumTable next
 *     - a specified number of Splits. Contain offsets to SampleHeaders
 * - area of consecutive SampleHdrs, pointing to sample data
 * - two uint32_t with value of 0, possible delimitator
 * - large block of ulaw-like encoded samples, up to offset from file header
 *
 * Specifications:
 * - sample data resembles u-law, except that bottom half is inverted
 * - most sample data is mono, some will be L-R interleaved pairs(specified by SampleHeaders)
 * - tuning is 8.8 fixed-point, 256 for every semitone
 * - volume of 256 is 100%, values above amplify(where does it stop?)
 * - envelope lengths(?)
*/


#include <stdint.h>

int16_t wg_pcmTable[256] = {
         0,     16,     32,     48,     64,     80,     96,    112,
       128,    144,    160,    176,    192,    208,    224,    240,
       256,    272,    288,    304,    320,    336,    352,    368,
       384,    400,    416,    432,    448,    464,    480,    496,
       512,    544,    576,    608,    640,    672,    704,    736,
       768,    800,    832,    864,    896,    928,    960,    992,
      1026,   1090,   1154,   1218,   1282,   1346,   1410,   1474,
      1539,   1603,   1667,   1731,   1795,   1859,   1923,   1987,
      2056,   2184,   2313,   2441,   2570,   2698,   2827,   2955,
      3084,   3212,   3341,   3469,   3598,   3726,   3855,   3983,
      4129,   4387,   4645,   4903,   5161,   5419,   5677,   5935,
      6193,   6451,   6709,   6967,   7225,   7483,   7741,   7999,
      8327,   8847,   9367,   9887,  10407,  10927,  11447,  11967,
     12487,  13007,  13527,  14047,  14567,  15087,  15607,  16127,
     16927,  17983,  19039,  20095,  21151,  22207,  23263,  24319,
     25375,  26431,  27487,  28543,  29599,  30655,  31711,  32767,
    -32768, -31744, -30720, -29696, -28672, -27648, -26624, -25600,
    -24576, -23552, -22528, -21504, -20480, -19456, -18432, -17408,
    -16384, -15872, -15360, -14848, -14336, -13824, -13312, -12800,
    -12288, -11776, -11264, -10752, -10240,  -9728,  -9216,  -8704,
     -8192,  -7936,  -7680,  -7424,  -7168,  -6912,  -6656,  -6400,
     -6144,  -5888,  -5632,  -5376,  -5120,  -4864,  -4608,  -4352,
     -4096,  -3968,  -3840,  -3712,  -3584,  -3456,  -3328,  -3200,
     -3072,  -2944,  -2816,  -2688,  -2560,  -2432,  -2304,  -2176,
     -2048,  -1984,  -1920,  -1856,  -1792,  -1728,  -1664,  -1600,
     -1536,  -1472,  -1408,  -1344,  -1280,  -1216,  -1152,  -1088, 
     -1024,   -992,   -960,   -928,   -896,   -864,   -832,   -800, 
      -768,   -736,   -704,   -672,   -640,   -608,   -576,   -544,
      -512,   -496,   -480,   -464,   -448,   -432,   -416,   -400,
      -384,   -368,   -352,   -336,   -320,   -304,   -288,   -272,
      -256,   -240,   -224,   -208,   -192,   -176,   -160,   -144,
      -128,   -112,    -96,    -80,    -64,    -48,    -32,    -16
};

#pragma pack(1)

typedef struct {
    char     magic[8]; //WgTPDHdr
    //3 bytes are size, 4th byte is a flag
    uint32_t fileSizeAndFlag;
    uint16_t bankVersion;
    uint8_t  unk01[2];
} wg_BankHeader;

typedef struct {
    int8_t   pcm[256];
} wg_JunkPCM;

typedef struct {
    uint32_t t[256];
} wg_PatchMap;

typedef struct {
    uint16_t volume;
    int16_t  tuning;
    int16_t  randPitch; //why is this signed?
    uint8_t  isDrumKit;
    uint8_t  unk01[7];
    uint16_t splitNum;
} wg_Patch;

typedef struct {
    uint8_t tab[128];
} wg_DrumTable;

typedef struct {
    uint8_t  rangeStart;
    uint8_t  rangeEnd;
    uint8_t  mapIndex;
    int8_t   pan;
    uint8_t  unk01[2];
    int16_t  tuning;
    uint32_t smpHeadOff;
} wg_Split;

typedef struct {
    uint32_t offStart;
    uint32_t offLoop;
    uint32_t offEnd;
    uint16_t volume;
    int16_t  tuning;
    uint16_t lenAttack;
    uint16_t lenDecay;
    uint16_t volSustain;
    uint16_t lenRelease;
    uint8_t  flags;
    uint8_t  unk01[3];
} wg_SampleHdr;

#pragma pack()

enum WG_FLAGS {
    WG_FLG_BIT1      = 1<<0,
    WG_FLG_BIT2      = 1<<1,
    WG_FLG_FIXEDNOTE = 1<<2, //plays fixed note
    WG_FLG_BIT4      = 1<<3,
    WG_FLG_BIT5      = 1<<4,
    WG_FLG_STEREO    = 1<<5, //interleaved int16_t
    WG_FLG_ATONAL    = 1<<6, //non-musical tuning
    WG_FLG_BIT8      = 1<<7
};

#endif