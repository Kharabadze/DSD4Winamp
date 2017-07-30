#pragma once
#define BlockLength 576
struct tDSD_decoder{
	int channels,x_factor,q_factor;
	bool LSB_first,MSB_first;
	int *table[256];
	int *queue;
	bool need_regenerate_table;
	//int total_volume;


	void set_ch_x(int ch,int xx);
	void set_LSB_MSB(bool LSB,bool MSB);
	void free_table(void);
	void generate_table(void);
	//void set_volume(int in_volume);

	//--- main part
	void decode_block(unsigned char **input_data,int **output_data);

	//--- dummy decoder
	__int64 dummy_filter[16];//maximum - 16 channels
	void dummy_block(unsigned char **input_data,int **output_data);

	tDSD_decoder(void);
	~tDSD_decoder(void);
};
