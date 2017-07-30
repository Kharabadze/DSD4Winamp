///////////////////////////
// Direct Stream Digital //
//  by David Kharabadze  //
//  License: LGPL v.3    //
///////////////////////////
// Samples are downloaded from:
// http://www.ayre.com/insights_dsdvspcm.htm
// (link from http://pcaudiophile.ru/index.php?id=31 )
///////////////////////////
// more sample:
// https://www.oppodigital.com/hra/dsd-by-davidelias.aspx
///////////////////////////

//------------------------ External headers
#include<Windows.h>
#include "../Winamp/in2.h"
#include<stdio.h>

//------------------------ Internal headers
#include"DSD.h"
#include"Decoder.h"

//------------------------ Global constants & parameters
// post this to the main window at end of file (after playback as stopped)
#define WM_WA_MPEG_EOF WM_USER+2
#define BPS 24 //16 //24
int SAMPLERATE=0;

//------------------------ Connection with plugin
In_Module mod;//Procedure addresses & parameters
int INIT_MODULE(void);
int unused_variable=INIT_MODULE();

//------------------------ Global variables start
FILE *debugfile=0;//Debug file for information output
FILE *f;//Input sound file
tDSD DSD;//DSD parser
tDSD_decoder DSD_decoder;
//------------------------ Buffers
int encoded_size=0,decoded_size=0,sample_size=0;
unsigned char **encoded_data=0;
int **decoded_data=0;
char *sample_data=0;//output

//int file_length;		// file length, in bytes

volatile int killDecodeThread=0;
HANDLE thread_handle=INVALID_HANDLE_VALUE;

__int64 decode_pos_samples;//current decoding position in samples (44100)
int decode_pos_ms;		// current decoding position, in milliseconds. 
						// Used for correcting DSP plug-in pitch changes
int paused;				// are we paused?
volatile int seek_needed; // if != -1, it is the point that the decode 




//------------------------ Function for size reduce
//BOOL WINAPI _DllMainCRTStartup(HANDLE hInst, ULONG ul_reason_for_call, LPVOID lpReserved){
//	return TRUE;
//}


//------------------------- Code start
void config(HWND hwndParent){
	MessageBox(hwndParent,
		L"No configuration. Auto-detection of DSD type",
		L"Configuration",MB_OK);
	// if we had a configuration box we'd want to write it here (using DialogBox, etc)
}

void about(HWND hwndParent){
	MessageBox(hwndParent,L"Direct Stream Digital Player 1.1, by David Kharabadze",
		L"About DSD Player",MB_OK);
}

void init() {
	debugfile=0;//OFF DEBUG
	//if(debugfile==0)fopen_s(&debugfile,"D:/David/DSDdebug.txt","wt");//ON DEBUG
	if(debugfile){fprintf(debugfile,"Start debug\n");fflush(debugfile);}
	return;
}

void quit() {
	//if((sample_buffer_size!=0)&&(sample_buffer!=0))
	//delete[] sample_buffer;
	//sample_buffer_size=0;
	//sample_buffer=0;

	if(debugfile){fprintf(debugfile,"Finish debug\n");fflush(debugfile);}
	if(debugfile)fclose(debugfile);
	return;
}

void getfileinfo(const char *filename, char *title, int *length_in_ms){
	title=0;//Dont't decode ID3v2 TAG
	if (!filename || !*filename){  // currently playing file
		if(debugfile){fprintf(debugfile,"Curent File Info\n");fflush(debugfile);}	
		if(length_in_ms)
			*length_in_ms=int(DSD.Samples*1000/DSD.SampleRate);
		if(title)
			strcpy_s(title,14,"Current_File");//???
		if(debugfile){fprintf(debugfile,"File info:%llu / %i\n",DSD.Samples,DSD.SampleRate);fflush(debugfile);}
	}else{
		if(debugfile){fprintf(debugfile,"File %s info\n",filename);fflush(debugfile);}
		tDSD *locDSD=new tDSD;
		FILE *lf;//lf=fopen(filename,"rb");
		fopen_s(&lf,filename,"rb");

		locDSD->start(lf);
		fclose(lf);
		if(length_in_ms)
			*length_in_ms=int(locDSD->Samples*1000/locDSD->SampleRate);
		if(title)
			strcpy_s(title,13,"Future_File");
		if(debugfile){fprintf(debugfile,"File info:%llu / %i\n",locDSD->Samples,locDSD->SampleRate);fflush(debugfile);}
		//locDSD->finish();
		delete locDSD;
		if(debugfile){fprintf(debugfile,"Local DSD finished\n");fflush(debugfile);}
	}
	if(debugfile){fprintf(debugfile,"Finish file info\n");fflush(debugfile);}
	return;
}

int infoDlg(const char *fn, HWND hwnd)
{
	// CHANGEME! Write your own info dialog code here
	return 0;
}

int isourfile(const char *fn) { 
// return !strncmp(fn,"http://",7); to detect HTTP streams, etc
	return 0; 
} 

//-------------------------------------------------------------------------- MAIN DECODER
// render 576 samples into buf. 
// this function is only used by DecodeThread. 

// note that if you adjust the size of sample_buffer, for say, 1024
// sample blocks, it will still work, but some of the visualization 
// might not look as good as it could. Stick with 576 sample blocks
// if you can, and have an additional auxiliary (overflow) buffer if 
// necessary.. 
int get_576_samples(char *buf){
	//int halfsize=576*DSD.Channels*BPS/8;
	__int64 l=0;
	if(debugfile){fprintf(debugfile,"Start DSD.GetSamples\n");fflush(debugfile);}
	l=DSD.get_samples(576*DSD_decoder.x_factor,encoded_data);//l=bytes in one channel
	if(debugfile){fprintf(debugfile,"Start DSD_decodr.decode_block\n");fflush(debugfile);}
	//DSD_decoder.dummy_block(encoded_data,decoded_data);
	DSD_decoder.decode_block(encoded_data,decoded_data);
	//l=l*Channels*(BPS/8)/DSD_decoder.x_factor/8;
	
	//int newsize=(l*8/DSD_decoder.x_factor)*DSD_decoder.channels*(BPS/8);//Exact value
	//int newsize=(l==0)?0:576*DSD_decoder.channels*(BPS/8);//zero or 576 samples
	int newsize=(l==576*DSD_decoder.x_factor/8)?576*DSD_decoder.channels*(BPS/8):0;//zero or 576 samples

	if(debugfile){fprintf(debugfile,"Start sound2char\n");fflush(debugfile);}
	int buk=0;
	for(int sm=0;sm<576;sm++){
		for(int ch=0;ch<DSD.Channels;ch++){
			for(int bte=0;bte<BPS;bte+=8){
				buf[buk++]=(decoded_data[ch][sm]>>bte)&0xff;
			}
		}
	}
	if(debugfile){fprintf(debugfile,"Finish 576-block\n");fflush(debugfile);}
	return (newsize);
}

int free_memory_bufer(void){
	if((encoded_size!=0)&&(encoded_data)){
		for(int ch=0;ch<DSD.Channels;ch++){
			if(encoded_data[ch]){
				delete[]encoded_data[ch];
				encoded_data[ch]=0;
			}
		}
		delete[]encoded_data;encoded_data=0;
		encoded_size=0;
	};
	if((decoded_size!=0)&&(decoded_data)){
		for(int ch=0;ch<DSD.Channels;ch++){
			if(decoded_data[ch]){
				delete[]decoded_data[ch];
				decoded_data[ch]=0;
			}
		}
		delete[]decoded_data;decoded_data=0;
		decoded_size=0;
	};
	if((sample_size!=0)&&(sample_data)){
		delete[]sample_data;sample_data=0;
		sample_size=0;
	}
	return 0;
}


DWORD WINAPI DecodeThread(LPVOID b)
{
	if(debugfile){fprintf(debugfile,"Start DecodeThread\n");fflush(debugfile);}
	int done=0; // set to TRUE if decoding has finished
	while (!killDecodeThread) 
	{
		if (seek_needed != -1){ // seek is needed.
			decode_pos_ms=seek_needed;

			decode_pos_samples=decode_pos_ms;
			decode_pos_samples=decode_pos_samples*SAMPLERATE/1000;

			__int64 offset=seek_needed;
			offset=offset*DSD.SampleRate/1000;

			seek_needed=-1;
			//if(debugfile){fprintf(debugfile,"Start rewind %i / %i\n",seek_needed,offset);fflush(debugfile);}
			DSD.rewindto(offset);
			//if(debugfile){fprintf(debugfile,"Finish rewind %i / %i\n",seek_needed,offset);fflush(debugfile);}
		}

		if (done){ // done was set to TRUE during decoding, signaling eof
			if(debugfile){fprintf(debugfile,"Done...\n");fflush(debugfile);}
			mod.outMod->CanWrite();		// some output drivers need CanWrite
									    // to be called on a regular basis.

			if (!mod.outMod->IsPlaying()) 
			{
				// we're done playing, so tell Winamp and quit the thread.
				PostMessage(mod.hMainWindow,WM_WA_MPEG_EOF,0,0);
				break;//return 0;	// quit thread
			}
			Sleep(10);		// give a little CPU time back to the system.
		}else if(paused){
			Sleep(10);
		}else{
			int bl=((576*DSD.Channels*(BPS/8))*(mod.dsp_isactive()?2:1));

			// CanWrite() returns the number of bytes you can write, so we check that
			// to the block size. the reason we multiply the block size by two if 
			// mod.dsp_isactive() is that DSP plug-ins can change it by up to a 
			// factor of two (for tempo adjustment).	
			if (mod.outMod->CanWrite() >= bl){
				if(debugfile){fprintf(debugfile,"Start get576samples %i\n",bl);fflush(debugfile);}
				int l=get_576_samples(sample_data);	   // retrieve samples

				if(debugfile){fprintf(debugfile,"Finish get576samples %i\n",l);fflush(debugfile);}
	
				if (!l){			// no samples means we're at eof
					done=1;
				}else{	// we got samples!
					// give the samples to the vis subsystems
					if(debugfile){fprintf(debugfile,"SA Add pcm data %i\n",decode_pos_ms);fflush(debugfile);}
					mod.SAAddPCMData((char *)sample_data,DSD.Channels,BPS,decode_pos_ms);
					if(debugfile){fprintf(debugfile,"VS Add pcm data %i\n",decode_pos_ms);fflush(debugfile);}
					mod.VSAAddPCMData((char *)sample_data,DSD.Channels,BPS,decode_pos_ms);
					// adjust decode position variable
					decode_pos_samples+=576; //(576*1000)/SAMPLERATE;
					decode_pos_ms=int((decode_pos_samples*1000)/SAMPLERATE);
					if(debugfile){fprintf(debugfile,"Finish Add pcm data %i\n",decode_pos_ms);fflush(debugfile);}
					// if we have a DSP plug-in, then call it on our samples
					if (mod.dsp_isactive()) 
						l=mod.dsp_dosamples(
						(short *)sample_data,2*576,BPS,DSD.Channels,SAMPLERATE
						  ) // dsp_dosamples
						  *(DSD.Channels*(BPS/8));

					// write the pcm data to the output system
					mod.outMod->Write(sample_data,l);
				}
			}else Sleep(20);
			// if we can't write data, wait a little bit. Otherwise, continue 
			// through the loop writing more data (without sleeping)
		}
	}
	if (f){fclose(f);f=0;}//for next opening...
	if(debugfile){fprintf(debugfile,"Finish DecodeThread\n");fflush(debugfile);}
	return 0;
}

// called when winamp wants to play a file
int play(const char *fn){
	if(debugfile){fprintf(debugfile,"Start Playing\n");fflush(debugfile);}
	int maxlatency;
	DWORD thread_id;

	//paused=0;
	//decode_pos_ms=0;
	//seek_needed=-1;
	paused=0;
	decode_pos_ms=0;
	seek_needed=-1;

		
	// CHANGEME! Write your own file opening code here
	//f=fopen(fn,"rb");
	fopen_s(&f,fn,"rb");
	if(f==0)return 1;

	//------------------------ New DSD+Decoder
	DSD.start(f);
	//int NCH=DSD.Channels;//Channels
	if(DSD.SampleRate==0) return 1;//Zero samplerate :)
	else if((DSD.SampleRate%44100)==0){SAMPLERATE=44100;DSD_decoder.set_ch_x(DSD.Channels,DSD.SampleRate/44100);}
	else if((DSD.SampleRate%48000)==0){SAMPLERATE=48000;DSD_decoder.set_ch_x(DSD.Channels,DSD.SampleRate/48000);}
	else return 1;//
	DSD_decoder.set_LSB_MSB(DSD.LSB_first,DSD.MSB_first);

	//------------------------ Get memory
	if(debugfile){fprintf(debugfile,"Getting memory\n");fflush(debugfile);}
	free_memory_bufer();

	encoded_size=576*DSD_decoder.x_factor/8;
	encoded_data=new unsigned char*[DSD.Channels];
	for(int ch=0;ch<DSD.Channels;ch++)encoded_data[ch]=new unsigned char[encoded_size];

	decoded_size=576;
	decoded_data=new int*[DSD.Channels];
	for(int ch=0;ch<DSD.Channels;ch++)decoded_data[ch]=new int[decoded_size];

	sample_size = DSD.Channels*576*2*BPS/8;
	sample_data = new char[sample_size];

	if(debugfile){fprintf(debugfile,"Finish Getting memory\n");fflush(debugfile);}
	//------------------------ Main part

	
	//strcpy(lastfn,fn); //???

	// -1 and -1 are to specify buffer and prebuffer lengths.
	// -1 means to use the default, which all input plug-ins should
	// really do.
	maxlatency = mod.outMod->Open(SAMPLERATE,DSD.Channels,BPS, -1,-1);

	// maxlatency is the maxium latency between a outMod->Write() call and
	// when you hear those samples. In ms. Used primarily by the visualization
	// system.
	if (maxlatency < 0){
		fclose(f);f=0;
		return 1;
	}

	// dividing by 1000 for the first parameter of setinfo makes it
	// display 'H'... for hundred.. i.e. 14H Kbps.
	mod.SetInfo((DSD.SampleRate*DSD.Channels)/1000,SAMPLERATE/1000,DSD.Channels,1);

	// initialize visualization stuff
	mod.SAVSAInit(maxlatency,SAMPLERATE);
	mod.VSASetInfo(SAMPLERATE,DSD.Channels);

	// set the output plug-ins default volume.
	// volume is 0-255, -666 is a token for
	// current volume.
	mod.outMod->SetVolume(-666); 
	
	decode_pos_samples=0;decode_pos_ms=0;
	// launch decode thread
	killDecodeThread=0;
	thread_handle = (HANDLE) 
		CreateThread(NULL,0,(LPTHREAD_START_ROUTINE) DecodeThread,NULL,0,&thread_id);
	
	return 0; 
}
void pause() { paused=1; mod.outMod->Pause(1); }
void unpause() { paused=0; mod.outMod->Pause(0); }
int ispaused() { return paused; }
void stop() { 
	if (thread_handle != INVALID_HANDLE_VALUE)
	{
		killDecodeThread=1;
		if (WaitForSingleObject(thread_handle,10000) == WAIT_TIMEOUT)
		{
			MessageBox(mod.hMainWindow,L"error asking thread to die!\n",
				L"error killing decode thread",0);
			TerminateThread(thread_handle,0);
		}
		CloseHandle(thread_handle);
		thread_handle = INVALID_HANDLE_VALUE;
	}

	// close output system
	mod.outMod->Close();

	// deinitialize visualization
	mod.SAVSADeInit();
	

	// CHANGEME! Write your own file closing code here
	if (f){fclose(f);f=0;}

	//--- Free buffer
	free_memory_bufer();
	//if((sample_buffer_size!=0)&&(sample_buffer!=0))
	//delete[] sample_buffer;
	//sample_buffer_size=0;
	//sample_buffer=0;

}

//-------------------------------------------------------------------------- sizes
// returns length of playing track (in ms)
int getlength() {
	return int(DSD.Samples*1000/DSD.SampleRate);
	//(file_length*10)/(SAMPLERATE/100*NCH*(BPS/8)); 
}


// returns current output position, in ms.
// you could just use return mod.outMod->GetOutputTime(),
// but the dsp plug-ins that do tempo changing tend to make
// that wrong.
int getoutputtime() { 
	return decode_pos_ms+
		(mod.outMod->GetOutputTime()-mod.outMod->GetWrittenTime()); 
}


// called when the user releases the seek scroll bar.
// usually we use it to set seek_needed to the seek
// point (seek_needed is -1 when no seek is needed)
// and the decode thread checks seek_needed.
void setoutputtime(int time_in_ms) { 
	seek_needed=time_in_ms; 
}

//-------------------------------------------------------------------------- sound processing
void setvolume(int volume) {

	if(debugfile){fprintf(debugfile,"Set Volume %i\n",volume);fflush(debugfile);}
	mod.outMod->SetVolume(volume); 
	//DSD_decoder.set_volume(volume);
	//mod.outMod->SetVolume(255); 
}
void setpan(int pan) { mod.outMod->SetPan(pan); }
void eq_set(int on, char data[10], int preamp){ 
	// most plug-ins can't even do an EQ anyhow.. I'm working on writing
	// a generic PCM EQ, but it looks like it'll be a little too CPU 
	// consuming to be useful :)
	// if you _CAN_ do EQ with your format, each data byte is 0-63 (+20db <-> -20db)
	// and preamp is the same. 
	return;
}



//--------------------------------------------------------------------------
int INIT_MODULE(void){
	
	//mod = {
	mod.version=IN_VER;	// defined in IN2.H
	mod.description="Direct Stream Digital Player v1.1 (x86)";
	mod.hMainWindow=0;	// hMainWindow (filled in by winamp)
	mod.hDllInstance=0;  // hDllInstance (filled in by winamp)
	mod.FileExtensions="DSF\0Sony Direct Stream Digital (*.DSF)\0DFF\0Phillips Direct Stream Digital (*.DFF)\0";
	// this is a double-null limited list. "EXT\0Description\0EXT\0Description\0" etc.
	mod.is_seekable=1;	// is_seekable
	mod.UsesOutputPlug=1;	// uses output plug-in system
	mod.Config=config;
	mod.About=about;
	mod.Init=init;
	mod.Quit=quit;
	mod.GetFileInfo=getfileinfo;
	mod.InfoBox=infoDlg;
	mod.IsOurFile=isourfile;
	mod.Play=play;
	mod.Pause=pause;
	mod.UnPause=unpause;
	mod.IsPaused=ispaused;
	mod.Stop=stop;
	
	mod.GetLength=getlength;
	mod.GetOutputTime=getoutputtime;
	mod.SetOutputTime=setoutputtime;

	mod.SetVolume=setvolume;
	mod.SetPan=setpan;

	//---- Visualization	0,0,0,0,0,0,0,0,0, // visualization calls filled in by winamp
	mod.SAVSAInit=0;
	mod.SAVSADeInit=0;
	mod.SAAddPCMData=0;
	mod.SAGetMode=0;
	mod.SAAdd=0;
	mod.VSAAddPCMData=0;
	mod.VSAGetMode=0;
	mod.VSAAdd=0;
	mod.VSASetInfo=0;

	//---- DSP  0,0, // dsp calls filled in by winamp
	mod.dsp_isactive=0;
	mod.dsp_dosamples=0;

	mod.EQSet=eq_set;

	mod.SetInfo=0;		// setinfo call filled in by winamp

	mod.outMod=0; // out_mod filled in by winamp

	return 0;
}

// exported symbol. Returns output module.

extern "C" __declspec( dllexport ) In_Module * winampGetInModule2()
{
	//INIT_MODULE();
	return &mod;
}
