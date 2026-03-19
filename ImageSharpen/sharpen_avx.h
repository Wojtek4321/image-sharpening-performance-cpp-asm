#pragma once

extern "C" void sharpen_avx(
    unsigned char* data_src,
    unsigned char* data_blur,
    unsigned char* data_dst,
    int pixel_count,
    double amount
);


extern "C" void box_blur_3x3_avx(
	unsigned char* data_src,
	unsigned char* data_dst,
	int width,
	int height
);
