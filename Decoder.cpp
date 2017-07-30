#include"Decoder.h"
#include<math.h>
#define MyPi 3.1415926535897932384626433832795

#include<stdio.h>		//+++++++DEBUG
extern FILE *debugfile;	//+++++++DEBUG

void tDSD_decoder::set_ch_x(int ch,int xx){
	channels=ch;
	x_factor=xx;
	//--- Dummy filter init
	for(int i=0;i<channels;i++)dummy_filter[i]=0;
	need_regenerate_table=true;
	return;
}
void tDSD_decoder::set_LSB_MSB(bool LSB,bool MSB){
	LSB_first=LSB;
	MSB_first=MSB;
	need_regenerate_table=true;
	return;
}
void tDSD_decoder::free_table(void){
	for(int i=0;i<256;i++)if(table[i]){
		delete[](table[i]);
		table[i]=0;
	}
	if(queue){
		delete[]queue;
		queue=0;
	}
	return;
}
void tDSD_decoder::generate_table(void){//32-bit variant
	int length=x_factor*q_factor;
	free_table();
	//--- get memory
	for(int bbb=0;bbb<256;bbb++){
		table[bbb]=new int[length/8];
	}
	queue=new int[q_factor*channels];//QUEUE :)
	double *realtable=new double[length];
	for(int i=0;i<q_factor*channels;i++)queue[i]=0;//QUEUE :)

	//--- fill memory (double)
	for(int i=0;i<length;i++){
		double fi=(i-length/2+0.5)/x_factor;
		realtable[i]=(sin(MyPi*fi)/fi)*sin(MyPi*(i+0.5)/length);
	}
	//--- renorm
	double sum=0.0;
	for(int i=0;i<length;i++){sum+=abs(realtable[i]);}
	for(int i=0;i<length;i++){realtable[i]*=2147483647.0/sum;}//In worthest case 32bit
	//--- fill memory (int)
	for(int ai=0;ai<q_factor;ai++){
		for(int bi=0;bi<x_factor;bi+=8){
			int i=ai*x_factor+bi;
			int i8=((q_factor-1-ai)*x_factor+bi)/8;
			for(int bbyte=0;bbyte<256;bbyte++){
				double locsum=0;
				for(int bbit_counter=0;bbit_counter<8;bbit_counter++){
					int bbit=(LSB_first)?bbit_counter:7-bbit_counter;
					locsum+=((bbyte>>bbit)&1)?realtable[i+bbit_counter]:-realtable[i+bbit_counter];
				}
				if(locsum>0){
					table[bbyte][i8]= int( locsum+0.00001);
				}else{
					table[bbyte][i8]=-int(-locsum+0.00001);
				}
			}
		}
	}
	//--- debug output
	if(debugfile){fprintf(debugfile,"Start Generate table\n");fflush(debugfile);}
	for(int i=0;i<length/8;i++){
		if(debugfile){fprintf(debugfile,"%i,",table[255][i]);fflush(debugfile);}
	}
	if(debugfile){fprintf(debugfile,"\n--\n");fflush(debugfile);}
	for(int i=0;i<length/8;i++){
		if(debugfile){fprintf(debugfile,"%i,",table[128][i]);fflush(debugfile);}
	}
	if(debugfile){fprintf(debugfile,"\nFinish Generate table\n");fflush(debugfile);}

	//--- free
	delete[]realtable;
	return;
}
//void  tDSD_decoder::set_volume(int in_volume){
//	total_volume=in_volume;
//	need_regenerate_table=true;
//	return;
//}

void tDSD_decoder::decode_block(unsigned char **input_data,int **output_data){
	if(need_regenerate_table){
		generate_table();
		need_regenerate_table=false;
	}

	int cs=0;
	for(int i=0;i<BlockLength;i++){
		for(int ch=0;ch<channels;ch++){
			for(int j=0;j<(x_factor/8);j++){
				unsigned char curb=input_data[ch][i*x_factor/8+j];
				for(int k=0;k<q_factor;k++){
					queue[k*channels+ch]+=table[curb][k*x_factor/8+j];
				}
			}
			//--- output
			int ou=queue[ch];
			//ou>>=8;//32 to 24
			ou=(ou>>7)+(ou>>8);//3x louder
			if(ou> 8388607)ou= 8388607;
			if(ou<-8388607)ou=-8388607;
			output_data[ch][i]=ou;
			//--- update
			for(int s=0,s2=ch;s<(q_factor-1);s++,s2+=channels)queue[s2]=queue[s2+channels];
			queue[(q_factor-1)*channels+ch]=0;
		}
	}
	return;
}
void tDSD_decoder::dummy_block(unsigned char **input_data,int **output_data){
	__int64 low_dum=9223372036854775807LL/(x_factor*x_factor);
	for(int ch=0;ch<channels;ch++){
		for(int i=0,ii=0;i<BlockLength;i++,ii+=x_factor/8){
			__int64 total=0;
			for(int j=0;j<(x_factor/8);j+=1){
				unsigned char curb=input_data[ch][ii+j];
				for(int bb=0;bb<8;bb++){
					int b2=(LSB_first)?bb:7-bb;
				//for(int bb=7;bb>=0;bb--)//Bad Variant
					//__int64 bit=((curb>>bb)&1)?low_dum:-low_dum;
					dummy_filter[ch]-=dummy_filter[ch]/x_factor;
					dummy_filter[ch]+=((curb>>b2)&1)?low_dum:-low_dum;
					total+=dummy_filter[ch];//64 bit
					//total+=((curb>>bb)&1)?511:-511;//16 bit
					//total+=((curb>>bb)&1)?131071:-131071;//24 bit
				}
			}
			//total>>=8;//64 bit->56 bit
			//total*=total_volume;//56 bit->64 bit
			//total>>=40;//64 bit->24 bit
			total>>=38;//little more loud
			if(total> 8388607)total= 8388607;
			if(total<-8388607)total=-8388607;
			output_data[ch][i]=(int)total;
		}
	}
	return;
}

tDSD_decoder::tDSD_decoder(void){
	q_factor=15;//Constant
	channels=0;
	x_factor=64;
	LSB_first=true;MSB_first=false;
	//total_volume=255;
	//--- table
	for(int i=0;i<256;i++)table[i]=0;
	need_regenerate_table=true;
	//--- reset queue
	queue=0;
	return;
}

tDSD_decoder::~tDSD_decoder(void){
	free_table();
	return;
}

/*
void tDSD_decoder::dummy_block(unsigned char **input_data,int **output_data){
	for(int ch=0;ch<channels;ch++){
		for(int i=0,ii=0;i<BlockLength;i++,ii+=x_factor/8){
			int total=0;
			for(int j=0;j<(x_factor/8);j+=1){
				unsigned char curb=input_data[ch][ii+j];
				for(int bb=0;bb<8;bb++){
					//total+=((curb>>bb)&1)?511:-511;//16 bit
					total+=((curb>>bb)&1)?131071:-131071;//24 bit
				}
			}
			output_data[ch][i]=total;
		}
	}
	return;
}
*/