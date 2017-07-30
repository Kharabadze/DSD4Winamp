//=====================================================
//=== (1) 1 bit corp.
//=== Structure for processing DSD file
//=====================================================
// FRM8 - file (Phillips) .DFF
// http://www.sonicstudio.com/pdf/dsd/DSDIFF_1.5_Spec.pdf
// http://dsdmaster.blogspot.ru/p/blog-page.html
//=====================================================
// DSD - file (Sony) .DSF
// http://dsd-guide.com/sites/default/files/white-papers/DSFFileFormatSpec_E.pdf
// http://www.ayre.com/insights_dsdvspcm.htm
// https://www.oppodigital.com/hra/dsd-by-davidelias.aspx
//=====================================================
// SACD = .DSF | .DFF
// http://minimserver.com/ug-library.html
// https://ru.wikipedia.org/wiki/%D0%A1%D1%80%D0%B0%D0%B2%D0%BD%D0%B5%D0%BD%D0%B8%D0%B5_%D1%86%D0%B8%D1%84%D1%80%D0%BE%D0%B2%D1%8B%D1%85_%D0%B0%D1%83%D0%B4%D0%B8%D0%BE%D1%84%D0%BE%D1%80%D0%BC%D0%B0%D1%82%D0%BE%D0%B2
//=====================================================
#include"DSD.h"
extern FILE *debugfile;

int Read64(__int64 *a,FILE *f){
	unsigned char c=0;
	*a=0;
	for(int i=0;i<8;i++){
		if(fread(&c,1,1,f)!=1)return 0;
		*a<<=8;
		*a|=c;
	}
	return 8;
}
int Read32(__int32 *a,FILE *f){
	unsigned char c=0;
	*a=0;
	for(int i=0;i<4;i++){
		if(fread(&c,1,1,f)!=1)return 0;
		*a<<=8;
		*a|=c;
	}
	return 4;
}
int Read16(__int16 *a,FILE *f){
	unsigned char c=0;
	*a=0;
	for(int i=0;i<2;i++){
		if(fread(&c,1,1,f)!=1)return 0;
		*a<<=8;
		*a|=c;
	}
	return 2;
}
int tDSD::start_DSD(void){
	if(debugfile){fprintf(debugfile,"Start DSD (Sony)\n");fflush(debugfile);}	
	//-------------------------------------- DSD chunk
	if(fread(&NAME,1,4,f)!=4)return 1;
	if(NAME!=' DSD')return 1;
	
	if(fread(&LocalSize,1,8,f)!=8)return 1;
	if((LocalSize-=12)<0)return 1;//Name+Size = 12 bytes
	
	if(fread(&GlobalSize,1,8,f)!=8)return 1;
	if((LocalSize-=8)<0)return 1;
	if((GlobalSize-=20)<0)return 1;//Name+Size+Size
	
	if(fread(&ID3Size,1,8,f)!=8)return 1;//IDPointer
	if((LocalSize-=8)<0)return 1;
	if((GlobalSize-=8)<0)return 1;
	if(ID3Size>0){
		if((ID3Size-=28)<0)return 1;
		ID3Size=GlobalSize-ID3Size;
		if(ID3Size>0)GlobalSize-=ID3Size;
	}

	if(LocalSize>0){//Skip extra
		//fseek(f,LocalSize,SEEK_CUR);//skip bytes
		_fseeki64(f,LocalSize,SEEK_CUR);//skip bytes
		GlobalSize-=LocalSize;
	}

	//-------------------------------------- FMT chunk
	if(fread(&NAME,1,4,f)!=4)return 1;
	if(NAME!=' tmf')return 1;
	if((GlobalSize-=4)<0)return 1;
	
	if(fread(&LocalSize,1,8,f)!=8)return 1;
	if((LocalSize-=12)<0)return 1;//Name+Size = 12 bytes
	if((GlobalSize-=8)<0)return 1;

	if(fread(&Version,1,4,f)!=4)return 1;
	if((LocalSize-=4)<0)return 1;//Name+Size = 12 bytes
	if((GlobalSize-=4)<0)return 1;
	if(Version!=1)return 1; //Unknown format

	if(fread(&FormatID,1,4,f)!=4)return 1;
	if((LocalSize-=4)<0)return 1;//Name+Size = 12 bytes
	if((GlobalSize-=4)<0)return 1;
	if(FormatID!=0)return 1; //Uncompresed only

	if(fread(&Sony_ChannelType,1,4,f)!=4)return 1;
	if((LocalSize-=4)<0)return 1;//Name+Size = 12 bytes
	if((GlobalSize-=4)<0)return 1;

	if(fread(&Sony_Channels,1,4,f)!=4)return 1;
	if((LocalSize-=4)<0)return 1;
	if((GlobalSize-=4)<0)return 1;
	Channels=Sony_Channels;
	if(debugfile){fprintf(debugfile,"Sony_Channels=%i\n",Sony_Channels);fflush(debugfile);}	

	if(fread(&SampleRate,1,4,f)!=4)return 1;
	if((LocalSize-=4)<0)return 1;
	if((GlobalSize-=4)<0)return 1;
	if(debugfile){fprintf(debugfile,"SampleRate=%i\n",SampleRate);fflush(debugfile);}	


	if(fread(&Sony_BPS,1,4,f)!=4)return 1;
	if((LocalSize-=4)<0)return 1;
	if((GlobalSize-=4)<0)return 1;
	LSB_first=MSB_first=false;
	if(Sony_BPS==1)LSB_first=true;
	else if(Sony_BPS==8)MSB_first=true;
	else return 1; //One bit only
	if(debugfile){fprintf(debugfile,"BPS=%i\n",Sony_BPS);fflush(debugfile);}	

	if(fread(&Samples,1,8,f)!=8)return 1;
	if((LocalSize-=8)<0)return 1;
	if((GlobalSize-=8)<0)return 1;
	if(debugfile){fprintf(debugfile,"Samples=%llu\n",Samples);fflush(debugfile);}


	if(fread(&BlockSize,1,4,f)!=4)return 1;
	if((LocalSize-=4)<0)return 1;
	if((GlobalSize-=4)<0)return 1;
	if(debugfile){fprintf(debugfile,"BlockSize=%i\n",BlockSize);fflush(debugfile);}

	unsigned __int32 Temp32;
	if(fread(&Temp32,1,4,f)!=4)return 1;
	if((LocalSize-=4)<0)return 1;
	if((GlobalSize-=4)<0)return 1;

	if(LocalSize>0){//Skip extra
		_fseeki64(f,LocalSize,SEEK_CUR);//skip bytes
		GlobalSize-=LocalSize;
	}

	//-------------------------------------- data chunk
	if(fread(&NAME,1,4,f)!=4)return 1;
	if(NAME!='atad')return 1;
	if((GlobalSize-=4)<0)return 1;
	
	if(fread(&LocalSize,1,8,f)!=8)return 1;
	if((LocalSize-=12)<0)return 1;//Name+Size = 12 bytes
	if((GlobalSize-=8)<0)return 1;

	//---------------------------
	StartData=ftell(f);//Start DATA
	//----------------------------

	__int64 msamples=LocalSize;
	msamples/=BlockSize*Sony_Channels;//make block
	msamples*=BlockSize*Sony_Channels;//make block

	{//correct last block
		__int64 last_block_start=StartData+msamples-BlockSize*Sony_Channels;
		_fseeki64(f,last_block_start,SEEK_SET);
		int minimum_last=BlockSize;
		for(int ch=0;ch<Sony_Channels;ch++){
			int last_non_eq=0;
			unsigned char last_byte=0;
			for(int sm=0;sm<BlockSize;sm++){
				unsigned char c=0;
				fread(&c,1,1,f);
				if((sm==0)||(c!=last_byte)){
					last_byte=c;
					last_non_eq=sm;
				}
			}
			if(last_non_eq<minimum_last)minimum_last=last_non_eq;
		}
		msamples-=BlockSize*Sony_Channels;
		msamples+=minimum_last*Sony_Channels;
	}

	msamples*=8;
	if(msamples<=Samples)Samples=msamples;


	//-------------------------------------- finish
	if(debugfile){fprintf(debugfile,"Finish DSD (Sony)\n");fflush(debugfile);}
	return 0;
}
int tDSD::start_FRM8(void){//Phillips format (fill it later)
	LSB_first=false;MSB_first=true;BlockSize=1;//Phillips standard

	//--- Read HEADER
	unsigned char c=0;
	__int64 GlobalSize,LocalSize,LocalLocalSize;
	__int32 NAME;
	//--- Start parcing
	if(fread(&NAME,1,4,f)!=4)return 0;//ERROR
	if(NAME!='8MRF')return 0;
	if(Read64(&GlobalSize,f)!=8)return 0;//ERROR
	if(fread(&NAME,1,4,f)!=4)return 0;//ERROR
	if(NAME!=' DSD')return 0;GlobalSize-=4;

	while(1){
		//--- Read local chank
		//printf("Global Size = %i\n",GlobalSize);
		if(GlobalSize==0)break;//Good finish
		if(fread(&NAME,1,4,f)!=4)return 0;GlobalSize-=4;
		if(Read64(&LocalSize,f)!=8)return 0;GlobalSize-=8;
		if(debugfile){fwrite(&NAME,1,4,debugfile);fflush(debugfile);}
		if(debugfile){fprintf(debugfile,"\n");fflush(debugfile);}
		if(debugfile){fprintf(debugfile,"Global Size = %i\n",GlobalSize);fflush(debugfile);}
		if(debugfile){fprintf(debugfile,"Local Size = %i\n",LocalSize);fflush(debugfile);}
		if((GlobalSize-=LocalSize)<0)return 0;//Too large chunk
		switch(NAME){
			case 'REVF':
				//printf("version ");
				while(LocalSize>0){
					if(fread(&c,1,1,f)!=1)return 0;LocalSize--;
					//printf((LocalSize)?"%i.":"%i DSDIFF\n",c);
				}
				break;
			case 'PORP'://break;
				if(fread(&NAME,1,4,f)!=4)return 0;LocalSize-=4;
				if(NAME!=' DNS')return 0;
				while(1){
					if(LocalSize==0)break;
					if(fread(&NAME,1,4,f)!=4)return 0;LocalSize-=4;
					if(Read64(&LocalLocalSize,f)!=8)return 0;LocalSize-=8;
					if((LocalSize-=LocalLocalSize)<0)return 0;//Too large chunk
					switch(NAME){
						case '  SF':
							if(Read32(&SampleRate,f)!=4)return 0;LocalLocalSize-=4;
							break;
						case 'LNHC':
							if(Read16(&Channels,f)!=2)return 0;LocalLocalSize-=2;
							ChannelName=new(unsigned __int32[Channels]);
							ChannelType=new(unsigned char[Channels]);
							for(int i=0;i<Channels;i++){
								ChannelName[i]=0;
								ChannelType[i]=CHANNEL_UNDEFINED;
							}
							int cur_ch;
							cur_ch=0;
							while(LocalLocalSize>=4){//loop on channels
								//printf("ch: %i\n",cur_ch);
								if(fread(&NAME,1,4,f)!=4)return 0;LocalLocalSize-=4;
								ChannelName[cur_ch]=NAME;
								for(int i=0;i<CHANNEL_MAX;i++)
									if(ChannelAbbrev[i]==NAME)ChannelType[cur_ch]=i;
								cur_ch++;
							}
							break;
						case 'RPMC':
							if(fread(&NAME,1,4,f)!=4)return 0;LocalLocalSize-=4;
							if(NAME!=' DSD')return 0;//Compressed data
							int len;
							len=0;
							if(fread(&len,1,1,f)!=1)return 0;LocalLocalSize--;
							CompressionName=new(unsigned char[len+1]);
							if(fread(CompressionName,1,len,f)!=len)return 0;LocalLocalSize-=len;
							CompressionName[len]=0;
							break;
						case 'SSBA':
							if(Read16(&StartHour,f)!=2)return 0;LocalLocalSize-=2;
							if(fread(&StartMinute,1,1,f)!=1)return 0;LocalLocalSize--;
							if(fread(&StartSecond,1,1,f)!=1)return 0;LocalLocalSize--;
							if(Read32(&StartSample,f)!=4)return 0;LocalLocalSize-=4;
							break;
						case 'OCSL':
							if(Read16(&SpeakerConfiguration,f)!=2)return 0;LocalLocalSize-=2;
							break;
						default:;
						//printf("Non-standard subheader \"%c%c%c%C\"\n",NAME&0xff,(NAME>>8)&0xff,(NAME>>16)&0xff,(NAME>>24)&0xff);
					}
					while(LocalLocalSize>0){
						if(fread(&c,1,1,f)!=1)return 0;LocalLocalSize--;
					}
				}
				break;
			case ' DSD':
				//printf("DSD sound data Gl=%i,Loc=%i\n",GlobalSize,LocalSize);
				StartData=ftell(f);//------------------------------ data start
				Samples=LocalSize/Channels*8;
				_fseeki64(f,LocalSize,SEEK_CUR);LocalSize=0;
				
				break;
			case ' TSD':
				return 0;//Compressed data
				break;
			case 'ITSD':
				break;
			case 'TMOC':
				break;
			case 'NIID':
				break;
			case 'FNAM':
				break;
			case ' 3DI'://ID3 tag
				break;
			default:;
				//printf("Non-standard header \"%c%c%c%C\"\n",NAME&0xff,(NAME>>8)&0xff,(NAME>>16)&0xff,(NAME>>24)&0xff);
		}
		while(LocalSize>0){
			if(fread(&c,1,1,f)!=1)return 0;LocalSize--;
		}

	}
	if(debugfile){fprintf(debugfile,"Good Finish of DFF file :)\n");fflush(debugfile);}
	return 0;
}
int tDSD::start(FILE *infile){// Sony or Phillips
	f=infile;
	//--- Get FileLength
	_fseeki64(f,0,SEEK_END);
	FileLength=_ftelli64(f);
	if(FileLength==0) return 1;
	fseek(f,0,SEEK_SET);


	//--- Read HEADER
	if(fread(&NAME,1,4,f)!=4)return 1;//ERROR
	fseek(f,0,SEEK_SET);//Goto start

	int ans=0;
	if(NAME=='8MRF'){
		ans=start_FRM8();
	}else if(NAME==' DSD'){
		ans=start_DSD();
	}else return 1;//ERROR

	//--- Start
	CurBlock=0;
	CurByte=0;
	if(ans==0){
		//-------------------------------------- buffer data
		bufersize=BlockSize*Channels;
		bufer=new unsigned char[bufersize];
		if(debugfile){fprintf(debugfile,"Goto data start: %i\n",StartData);fflush(debugfile);}
		_fseeki64(f,StartData,SEEK_SET);
	}else{
		bufer=0;
	}

	return ans;
}

int tDSD::get_block(void){//return:bytes
	__int64 maxbytes=(Samples*Channels/8)-CurBlock*BlockSize*Channels;//bytes left
	int need_bytes=(bufersize<maxbytes)?bufersize:maxbytes;
	int sz=0;
	if(need_bytes)sz=fread(bufer,1,need_bytes,f);
	for(int i=sz;i<bufersize;i++)bufer[i]=0x55;
	return sz;
}
int tDSD::get_samples(int need_sam,unsigned char **data){//return bytes
	int need_bytes=need_sam/8;
	int bytes_ready=0;
	int blsize=0;
	while(bytes_ready<need_bytes){
		//if(debugfile){fprintf(debugfile,"bytes_ready=%i,need_bytes=%i\n",bytes_ready,need_bytes);fflush(debugfile);}
		if(CurByte==0){
			blsize=get_block();CurBlock++;
			if(blsize==0)break;
		}
		int maxbyte=CurByte+(need_bytes-bytes_ready);//maximum sample in block
		if(maxbyte>BlockSize)maxbyte=BlockSize;//Limit
		for(int ch=0;ch<Channels;ch++){
			for(int by=CurByte,bb_i=0;by<maxbyte;by++,bb_i++){
				//if(debugfile){fprintf(debugfile,"%i,",bb_i);fflush(debugfile);}
				data[ch][bytes_ready+bb_i]=bufer[ch*BlockSize+by];
			}
			//if(debugfile){fprintf(debugfile,"\n",bytes_ready,CurByte);fflush(debugfile);}

		}
		bytes_ready+=maxbyte-CurByte;
		CurByte=maxbyte;
		if(CurByte>=BlockSize)CurByte=0;//Next Block
		//if(debugfile){fprintf(debugfile,"bytes_ready=%i,CurByte=%i\n",bytes_ready,CurByte);fflush(debugfile);}
	}
	if(debugfile){fprintf(debugfile,"+END+ bytes_ready=%i,need_bytes=%i\n",bytes_ready,need_bytes);fflush(debugfile);}
	return bytes_ready;//(no_empty)?outbytes:0;
}
int tDSD::rewindto(__int64 cursample){
	if(cursample>=Samples)return 1;//too high sample
	__int64 csb=cursample/8;//byte
	CurBlock=(csb/BlockSize);
	CurByte=(csb%BlockSize);
	__int64 fbeg=StartData+CurBlock*BlockSize*Channels;
	_fseeki64(f,fbeg,SEEK_SET);
	return 0;
}

int tDSD::finish(void){
	if(ChannelName)delete[](ChannelName);ChannelName=0;
	if(ChannelType)delete[](ChannelType);ChannelType=0;
	if(CompressionName)delete[](CompressionName);CompressionName=0;
	if(bufer)delete[](bufer);bufer=0;
	return 0;
}
tDSD::tDSD(void){
	ChannelName=0;
	ChannelType=0;
	CompressionName=0;
	bufer=0;
	return;
}
tDSD::~tDSD(void){
	finish();
	return;
}