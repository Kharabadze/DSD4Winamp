//=====================================================
//=== (1) 1 bit corp.
//=== Structure for processing DSD file
//=====================================================

#pragma once

#include<stdio.h>

//#include"Culture.h"
#define CHTYPE_MONO			1	MONO
#define CHTYPE_STEREO		2	FL FR
#define CHTYPE_3CHANNELS	3	FL FR CENTER
#define CHTYPE_QUADRO		4	FL FR BL BR
#define CHTYPE_4CHANNELS	5	FL FR CENTER SUB
#define CHTYPE_5CHANNELS	6	FL FR CENTER BL BR
#define CHTYPE_5_1			7	FL FR CENTER BL BR SUB
//=== Channel number
#define CHANNEL_LEFT  0
#define CHANNEL_RIGHT 1
#define CHANNEL_FRONT_LEFT  2
#define CHANNEL_FRONT_RIGHT 3
#define CHANNEL_CENTER 4
#define CHANNEL_SUB 5
#define CHANNEL_BACK_LEFT 6
#define CHANNEL_BACK_RIGHT 7
#define CHANNEL_UNDEFINED 255
#define CHANNEL_MAX 8 //Maximum channel number

const unsigned int ChannelAbbrev[CHANNEL_MAX]={'TFLS','TGRS','TFLM','TGRM','   C',' EFL','  SL','  SR'};


struct tDSD{
	//--- External data
	__int64 FileLength;
	__int32 SampleRate;
	__int16 Channels;
	__int64 Samples;
	__int32 BlockSize;//Per Channel	
	__int64 StartData;//Pointer to data
	bool LSB_first,MSB_first;

	//--- Phillips Specific
	unsigned __int32 *ChannelName;//Original Name
	unsigned char *ChannelType;//Decoded Type
	unsigned char *CompressionName;
	__int16 StartHour;
	__int8 StartMinute;
	__int8 StartSecond;
	__int32 StartSample;
	__int16 SpeakerConfiguration;

	//--- Sony specific
	unsigned __int32 Sony_ChannelType;
	unsigned __int32 Sony_Channels;
	unsigned __int32 Sony_BPS;

	//--- Internal data
	FILE *f;
	unsigned char c;
	__int64 GlobalSize,LocalSize,LocalLocalSize;
	__int64 ID3Size;
	unsigned __int32 NAME;
	__int32 Version,FormatID;

	//--- Pointer data & input bufer
	__int64 CurBlock;
	int CurByte;
	unsigned char *bufer;
	int bufersize;

	//--- Sound Data
	//unsigned __int64 SoundFrames;
	//unsigned char **SoundData;//Pointers
	//--- Additional
	//unsigned char PerCent;//Percent done
	//bool is_5_1;
	//---
	int start_DSD(void);//internal
	int start_FRM8(void);//internal
	int start(FILE *infile);

	int get_block(void);//internal subroutine
	int get_samples(int need_sam,unsigned char **data);
	int rewindto(__int64 cursample);

	int finish(void);
	//---
	tDSD(void);
	~tDSD(void);
};
