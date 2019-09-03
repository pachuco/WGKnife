#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "common.h"
#include "wgbank.h"
#include "wavfile.h"
#include "names.h"

#define MIDIMAP_OFF sizeof(wg_BankHeader) + 16*sizeof(wg_JunkPCM)
#define CENTP "%.2f"
#define MAXPATH 260
#define PATNAMES b00A5_patNames
#define SMPNAMES b00A5_smpNames

#define TABWIDTH 4
void t_fprintf(int nTabs, FILE* out, const char* format, ...) {
    va_list args;
    
    fprintf(out, "%*.s", TABWIDTH*nTabs, "");
    va_start(args, format);
    vfprintf(out, format, args);
    va_end(args);
}

int checkWgbankHeader(void* base, size_t len) {
    wg_BankHeader* head;
    
    head = base;
    if (!strcmp(head->magic, "WgTPDHdr")) return 0;
    if ((head->fileSizeAndFlag & 0x00FFFFFF) > len) return 0;
    
    return 1;
}

void getSampleName(char* outName, wg_SampleHdr* smpHdr, SampleIdent* identTab) {
    for (int i=0; identTab[i].offStart !=0 && identTab[i].offEnd; i++) {
        if ((identTab[i].offStart == smpHdr->offStart) && (identTab[i].offEnd == smpHdr->offEnd)) {
            sprintf(outName, "%s", identTab[i].name);
            return;
        }
    }
    
    sprintf(outName, "%010u-%010u-%08X-%08X", smpHdr->offStart, smpHdr->offEnd, smpHdr->offStart, smpHdr->offEnd);
}

#define SAMPLE_RATE     (22050)
#define BITS_PER_SAMPLE (16)
int writeWav(wg_SampleHdr* smpHdr, void* base, char* dest) {
    wav_FileHeader wFileHdr;
    wav_FormatHeader wFormHdr;
    wav_DataHeader wDataHdr;
    wav_SmplHeader wSmpHdr;
    wav_SampleLoop wSmpLoop;
    FILE* fout = 0;
    int16_t* outBuf = 0;
    uint8_t* inBuf;
    
    int numLoops     = smpHdr->offLoop?1:0;
    int numChannels  = smpHdr->flags & WG_FLG_STEREO ? 2 : 1;
    size_t smpSize   = numChannels * BITS_PER_SAMPLE/8;
    size_t smpLen    = (smpHdr->offEnd - smpHdr->offStart) / numChannels;
    size_t outBufLen = smpLen * smpSize;
    
    if (!(outBuf = malloc(outBufLen))) goto ERR;
    if (!(fout = fopen(dest, "wb"))) goto ERR;
    inBuf = (uint8_t*)((char*)base + smpHdr->offStart);
    
    for (unsigned int i=0; i < smpLen * numChannels; i++) {
        outBuf[i] = wg_pcmTable[inBuf[i]];
    }
    
    wFileHdr.id_RIFF    = IFFID_RIFF;
    wFileHdr.filesize   = 4 + sizeof(wav_FormatHeader) + sizeof(wav_DataHeader);
    wFileHdr.filesize  += sizeof(wav_SmplHeader)+ numLoops*sizeof(wav_SampleLoop) + outBufLen;
    wFileHdr.id_WAVE    = IFFID_WAVE;
    
    wFormHdr.id_fmt         = IFFID_fmt;
    wFormHdr.hdrlen         = 16;
    wFormHdr.format         = 1;
    wFormHdr.channels       = numChannels;
    wFormHdr.freqHz         = SAMPLE_RATE;
    wFormHdr.bytessec       = SAMPLE_RATE * smpSize;
    wFormHdr.samplesize     = smpSize;
    wFormHdr.bitspersample  = BITS_PER_SAMPLE;
    
    wDataHdr.id_data = IFFID_data;
    wDataHdr.length  = outBufLen;
    
    wSmpHdr.smpl_id         = IFFID_smpl;
    wSmpHdr.smpl_len        = sizeof(wav_SmplHeader) + sizeof(wav_SampleLoop) - 4;
    wSmpHdr.dwManufacturer  = 0;
    wSmpHdr.dwProduct       = 0;
    wSmpHdr.dwSamplePeriod  = 1000000000 / SAMPLE_RATE;
    //not quite correct
    wSmpHdr.dwBaseNote      = 60;//60 - ((smpHdr->tuning - 1)/256); //60
    //positive only pitch correction
    wSmpHdr.dwPitchFraction = 0;//smpHdr->tuning << 24; //0
    wSmpHdr.dwSMPTEFormat   = 0;
    wSmpHdr.dwSMPTEOffset   = 0;
    wSmpHdr.dwSampleLoops   = numLoops; // number of Loop structs
    wSmpHdr.cbSamplerData   = 0;
    
    if (wSmpHdr.dwPitchFraction && smpHdr->tuning < 0) wSmpHdr.dwBaseNote--;
    
    if (numLoops) {
        wSmpLoop.dwIdentifier   = 0x00000000;
        wSmpLoop.dwLoopType     = 0;
        wSmpLoop.dwLoopStart    = (smpHdr->offLoop - smpHdr->offStart) / numChannels;
        wSmpLoop.dwLoopEnd      = (smpHdr->offEnd  - smpHdr->offStart) / numChannels;
        wSmpLoop.dwFraction     = 0;
        wSmpLoop.dwPlayCount    = 0;
    }
    
    fwrite(&wFileHdr, sizeof(wav_FileHeader), 1, fout);
    fwrite(&wFormHdr, sizeof(wav_FormatHeader), 1, fout);
    fwrite(&wDataHdr, sizeof(wav_DataHeader), 1, fout);
    fwrite(outBuf, smpSize, smpLen, fout);
    fwrite(&wSmpHdr,  sizeof(wav_SmplHeader), 1, fout);
    fwrite(&wSmpLoop, sizeof(wav_SampleLoop), numLoops, fout);
    
    free(outBuf);
    fclose(fout);
    return 1;
    ERR:
        if (outBuf) free(outBuf);
        if (fout) fclose(fout);
        return 0;
}

//-----------------------------------------------
//DESCRIBE

void describeUnkBytes(int nTabs, FILE* out, void* base, void* p, size_t len) {
    unsigned int off = p - base;
    
    for (unsigned int i=0; i<len; i++) {
        uint8_t u8 = ((uint8_t*)p)[i]; 
        
        t_fprintf(nTabs, out, "???? %02Xh, U8 %3u, I8 %4d ", u8, u8, (int8_t)u8);
        if (!((off+i)&1) && len-i >= 2) {
            uint16_t u16 = *(uint16_t*)((char*)p+i);
            
            fprintf(out, "-┬-> ");
            fprintf(out, "%04Xh, U16 %5u, I16 %6d", u16, u16, (int16_t)u16);
        } else {
            if (i != 0) fprintf(out, "-┘");
        }
        fprintf(out, "\n");
    }
}

void describeSampleHdr(int nTabs, FILE* out, void* base, wg_SampleHdr* p) {
    char sampleName[32];
    char* flagNameTable[8] = {
        "WG_FLG_BIT1", "WG_FLG_BIT2",    "WG_FLG_FIXEDNOTE", "WG_FLG_BIT4",
        "WG_FLG_BIT5", "WG_FLG_STEREO",  "WG_FLG_ATONAL",    "WG_FLG_BIT8"
    };
    uint32_t lenLoop = (p->offEnd - p->offLoop)  / (p->flags & WG_FLG_STEREO ? 2 : 1);
    uint32_t lenSamp = (p->offEnd - p->offStart) / (p->flags & WG_FLG_STEREO ? 2 : 1);
    
    getSampleName(sampleName, p, SMPNAMES);
    t_fprintf(nTabs, out, "Sample known as \"%s.wav\"\n", sampleName);
    
    t_fprintf(nTabs, out, "* U32 Sample start: %08Xh\n", p->offStart);
    if (!p->offLoop) {
        t_fprintf(nTabs, out, "* U32 Sample loop:  DISABLED\n");
    } else {
        t_fprintf(nTabs, out, "* U32 Sample loop:  %08Xh (loop len %u samples)\n", p->offLoop, lenLoop);
    }
    t_fprintf(nTabs, out, "* U32 Sample end:   %08Xh (samp len %u samples)\n", p->offEnd, lenSamp);
    t_fprintf(nTabs, out, "* U16 Volume: %u\n", p->volume);
    t_fprintf(nTabs, out, "* I16 Relative tuning: "CENTP" semitones\n", (float)p->tuning / 0x100);
    t_fprintf(nTabs, out, "* U16 Envelope attack  len: %f\n", (float)p->lenAttack/64);
    t_fprintf(nTabs, out, "* U16 Envelope decay   len: %f\n", (float)p->lenDecay/64);
    t_fprintf(nTabs, out, "* U16 Envelope sustain volume: %u\n", p->volSustain);
    t_fprintf(nTabs, out, "* U16 Envelope release len: %f\n", (float)p->lenRelease/64);
    t_fprintf(nTabs, out, "* U8  Flags:\n");
    for (unsigned int i=0; i<8; i++) {
        if (p->flags & (1<<i)) t_fprintf(nTabs+1, out, "%s\n", flagNameTable[i]);
    }
    describeUnkBytes(nTabs, out, base, &p->unk01, 3);
}

void describeSplit(int nTabs, FILE* out, void* base, wg_Split* p, size_t len) {
    for (unsigned int i=0; i < len; i++) {
        t_fprintf(nTabs, out, "Split nr: %u\n", i);
        
        t_fprintf(nTabs, out, "* 2xU8 Note range: %u-%u\n", p->rangeStart, p->rangeEnd);
        t_fprintf(nTabs, out, "* U8   Drum map index: %u\n", p->mapIndex);
        t_fprintf(nTabs, out, "* I8   Panning: %i\n", p->pan);
        describeUnkBytes(nTabs, out, base, &p->unk01, 2);            
        t_fprintf(nTabs, out, "* I16  Relative tuning: "CENTP" semitones\n", (float)p->tuning / 0x100);
        t_fprintf(nTabs, out, "* U32  Sample header offset: %08Xh\n", p->smpHeadOff);
        
        t_fprintf(nTabs, out, "Sample header:\n");
        describeSampleHdr(nTabs+1, out, base, base + p->smpHeadOff);
        
        p++;
        fprintf(out, "\n");
    }
}

void describePatch(int nTabs, FILE* out, void* base, wg_Patch* p) {
    wg_DrumTable* dmap   = (wg_DrumTable*)((char*)p + sizeof(wg_Patch));
    wg_Split*     spBase = (wg_Split*)    ((char*)p + sizeof(wg_Patch) + (p->isDrumKit ? 128 : 0));
    
    if (!p->volume) {
        t_fprintf(nTabs, out, "DUMMY PATCH\n");
        return;
    }
    
    t_fprintf(nTabs, out, "* U16 Volume: %u\n", p->volume);
    t_fprintf(nTabs, out, "* I16 Relative tuning: "CENTP" semitones\n", (float)p->tuning / 0x100);
    t_fprintf(nTabs, out, "* I16 Pitch randomization: "CENTP" semitones\n", (float)p->randPitch / 0x100);
    t_fprintf(nTabs, out, "* U8  Is drumkit: %u\n", p->isDrumKit);
    describeUnkBytes(nTabs, out, base, &p->unk01, 7);
    t_fprintf(nTabs, out, "* U16 Split number: %u\n", p->splitNum);
    
    if (p->isDrumKit) {
        t_fprintf(nTabs, out, "Drum map:");
        for (unsigned int i=0; i<128; i++) {
            if (i%8 == 0) {
                fprintf(out, "\n");
                t_fprintf(nTabs+1, out, "");
            }
            fprintf(out, "%3u ", dmap->tab[i]);
        }
        fprintf(out, "\n");
    }
    
    t_fprintf(nTabs, out, "Splits:\n");
    describeSplit(nTabs+1, out, base, spBase, p->splitNum);
}

void describeHeader(int nTabs, FILE* out, void* base, wg_BankHeader* p) {
    t_fprintf(nTabs, out, "Wingroove bank\n");
    t_fprintf(nTabs, out, "* U24 File size: %06Xh\n", p->fileSizeAndFlag & 0x00FFFFFF);
    describeUnkBytes(nTabs, out, base, (char*)&p->fileSizeAndFlag + 3, 1);
    t_fprintf(nTabs, out, "* U16 Bank version %04Xh\n", p->bankVersion);
    describeUnkBytes(nTabs, out, base, &p->unk01, 2);
    
    fprintf(out, "\n");
}

int describeWgbank(FILE* out, void* base, size_t len) {
    wg_PatchMap* midiMap = (wg_PatchMap*)((char*)base + MIDIMAP_OFF);
    
    describeHeader(0, out, base, base);
    
    for (unsigned int i=0; i < 256; i++) {
        wg_Patch* patch = base + midiMap->t[i];
        
        t_fprintf(0, out,  "Preset: %03d:%03d %s\n", i<128?0:128, i&127, PATNAMES[i]);
        t_fprintf(0, out,  "U32 structure offset: %08Xh\n", midiMap->t[i]);
        describePatch(1, out, base, patch);
        
        fprintf(out, "\n\n\n\n");
    }
    
    
    return 1;
}
//-----------------------------------------------
//SAMPDUMP

#define SMP_SUF "_dmp"
void dumpSamples(char* name, void* base, size_t len) {
    wg_PatchMap* midiMap = (wg_PatchMap*)((char*)base + MIDIMAP_OFF);
    char outName[MAXPATH];
    int doWrite = 0;
    
    //!sloppy
    sprintf(outName, "%s"SMP_SUF, name);
    makeDir(outName);
    
    //we want to delete files first, then write new ones
    while (doWrite < 2) {
        for (unsigned int i=0; i < 256; i++) {
            wg_Patch* patch  = (wg_Patch*)((char*)base + midiMap->t[i]);
            wg_Split* spBase = (wg_Split*)((char*)patch + sizeof(wg_Patch) + (patch->isDrumKit ? 128 : 0));
            
            if (!patch->volume) continue;
            
            for (unsigned int j=0; j < patch->splitNum; j++) {
                char sampleName[64];
                wg_Split*     split  = &spBase[j];
                wg_SampleHdr* smpHdr = (wg_SampleHdr*)((char*)base + split->smpHeadOff);
                
                getSampleName(sampleName, smpHdr, SMPNAMES);
                sprintf(outName, "%s"SMP_SUF"/%s.wav", name, sampleName);
                
                if (doWrite) {
                    if(!isFileExist(outName)) writeWav(smpHdr, base, outName);
                } else {
                    //good enough
                    remove(outName);
                }
                
            }
        }
        doWrite++;
    }
}

//-----------------------------------------------
//SFZDUMP

int toSfzKeytrack(uint8_t flags) {
    return (flags & WG_FLG_FIXEDNOTE ? 0 : (flags & WG_FLG_ATONAL ? 50 : 100));
}
float toSfzPan(int8_t pan) {
    return ((float)pan * 100) / (pan>=0 ? 127 : 128);
}
int toSfzTuneKey(int16_t tune) {
    return 12 + (int)(tune-1)/256;
}
int toSfzTuneCent(int16_t tune) {
    return (((int)tune&0xFF) * (tune>=0?1:-1)) * 100 / 256;
}
float toSfzEnvelope(uint16_t env) {
    //bad
    return (float)env/64;
}
float toSfzAmpEGVolume(uint16_t vol) {
    return (float)vol * 100 / 256;
}

#define SFZ_SUF "_sfz"
void dumpSfz(char* name, void* base, size_t len) {
    wg_PatchMap* midiMap = (wg_PatchMap*)((char*)base + MIDIMAP_OFF);
    char outName[MAXPATH];
    int doWrite = 0;
    
    //!sloppy
    sprintf(outName, "%s"SFZ_SUF, name);
    makeDir(outName);
    sprintf(outName, "%s"SFZ_SUF"/samples", name);
    makeDir(outName);
    sprintf(outName, "%s"SFZ_SUF"/mel", name);
    makeDir(outName);
    sprintf(outName, "%s"SFZ_SUF"/drm", name);
    makeDir(outName);
    
    while (doWrite < 2) {
        for (unsigned int i=0; i < 256; i++) {
            wg_Patch* patch  = (wg_Patch*)((char*)base + midiMap->t[i]);
            wg_Split* spBase = (wg_Split*)((char*)patch + sizeof(wg_Patch) + (patch->isDrumKit ? 128 : 0));
            FILE* sfzout = NULL;
            
            if (!patch->volume) continue;
            sprintf(outName, "%s"SFZ_SUF"/%s/%03d %03d %s.sfz", name, i>>7?"drm":"mel", i&127, i&127, PATNAMES[i]);
            if (doWrite) {
                sfzout = fopen(outName, "w");
                if (!sfzout) continue;
                //printf("Patch: %03d:%03d %s\n", 128*(i>>7), i&127, PATNAMES[i]);
                
                fprintf(sfzout,
                    "//SFZ exported by WG-Knife, version 0.00000000000001\n"
                    "\n"
                    "<group>\n"
                    "//-----\n"
                    "\n"
                    "\n"
                );
                
            } else {
                remove(outName);
            }
            
            for (unsigned int j=0; j < patch->splitNum; j++) {
                char sampleName[64];
                wg_Split*     split  = &spBase[j];
                wg_SampleHdr* smpHdr = (wg_SampleHdr*)((char*)base + split->smpHeadOff);
                
                getSampleName(sampleName, smpHdr, SMPNAMES);
                sprintf(outName, "%s"SFZ_SUF"/samples/%s.wav", name, sampleName);
                
                if (doWrite) {
                    int numChans = smpHdr->flags & WG_FLG_STEREO ? 2 : 1;
                    uint32_t loopStart = (smpHdr->offLoop - smpHdr->offStart) / numChans;
                    uint32_t loopEnd   = (smpHdr->offEnd  - smpHdr->offStart) / numChans;
                    
                    if(!isFileExist(outName)) writeWav(smpHdr, base, outName);
                    
                    fprintf(sfzout,
                        "<region>\n"
                        "sample=../samples/%s.wav\n"
                        "lokey=%d hikey=%d\n"
                        "pitch_keytrack=%i\n"
                        "transpose=%i\n"
                        "tune=%i\n"
                        "pan=%f\n"
                        "loop_mode=%s\n",
                        sampleName,
                        split->rangeStart, split->rangeEnd,
                        toSfzKeytrack(smpHdr->flags),
                        toSfzTuneKey(smpHdr->tuning + split->tuning),
                        toSfzTuneCent(smpHdr->tuning + split->tuning),
                        toSfzPan(split->pan),
                        smpHdr->offStart ? "loop_continuous" : "no_loop"
                    );
                    
                    if (smpHdr->offLoop) fprintf(sfzout, "loop_start=%d loop_end=%d\n", loopStart, loopEnd);
                    
                    if(smpHdr->lenAttack)  fprintf(sfzout, "ampeg_attack=%f\n",  toSfzEnvelope(smpHdr->lenAttack));
                    if(smpHdr->lenDecay)   fprintf(sfzout, "ampeg_decay=%f\n",   toSfzEnvelope(smpHdr->lenDecay));
                    if(smpHdr->volSustain) fprintf(sfzout, "ampeg_sustain=%f\n", toSfzAmpEGVolume(smpHdr->volSustain));
                    if(smpHdr->lenRelease) fprintf(sfzout, "ampeg_release=%f\n", toSfzEnvelope(smpHdr->lenRelease));
                    
                    fprintf(sfzout, "\n");
                } else {
                    //good enough
                    remove(outName);
                }
            }
            if (sfzout) fclose(sfzout);
        }
        doWrite++;
    }
}

//-----------------------------------------------
//MAIN

#define ERR(X) {err=X;goto _ERR;};
int main(int argc, char *argv[]) {
    uint8_t* buf = 0;
    size_t buflen;
    int err;
    
    if( argc < 3 ) {
        printf(
            "usage:\n"
            "  wgknife -ARG FILENAME\n"
            "arguments:\n"
            "  -d: File description\n"
            "    Verbose description of file format.\n"
            "\n"
            "  -sd: Sample dump.\n"
            "    Dump all samples in a folder named after input.\n"
            "  -sfz: Create SFZ.\n"
            "    Outputs all instruments as SFZ in a folder named after input.\n"
        );
        ERR(1);
    }
    
    buf = loadfile(argv[2], &buflen);
    if (!buf) ERR(2);
    if (!checkWgbankHeader(buf, buflen)) ERR(3);
    
    #define C(X) (!strcmp(argv[1], X) && strlen(argv[1]) == sizeof(X)-1)
    if        (C("-sfz")) {
        dumpSfz(argv[2], buf, buflen);
    } else if (C("-sd")) {
        dumpSamples(argv[2], buf, buflen);
    } else if (C("-d")) {
        if (!describeWgbank(stdout, buf, buflen)) ERR(4);
    } else {
        printf("Unknown argument.\n");
        ERR(1);
    }
    #undef C
    
    return 0;
    _ERR:
        if (buf) free(buf);
        return err;
}