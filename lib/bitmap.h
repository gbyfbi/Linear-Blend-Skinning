//
// bitmap.h
//
// header file for MS bitmap format
//
//

#ifndef BITMAP_H
#define BITMAP_H

#include <stdio.h>
#include <string.h>

#define BMP_BI_RGB        0L

typedef unsigned short	BMP_WORD; 
typedef unsigned int	BMP_DWORD; 
typedef int				BMP_LONG; 
 
typedef struct { 
	BMP_WORD	bfType; 
	BMP_DWORD	bfSize; 
	BMP_WORD	bfReserved1; 
	BMP_WORD	bfReserved2; 
	BMP_DWORD	bfOffBits; 
} BMP_BITMAPFILEHEADER; 
 
typedef struct { 
	BMP_DWORD	biSize; 
	BMP_LONG	biWidth; 
	BMP_LONG	biHeight; 
	BMP_WORD	biPlanes; 
	BMP_WORD	biBitCount; 
	BMP_DWORD	biCompression; 
	BMP_DWORD	biSizeImage; 
	BMP_LONG	biXPelsPerMeter; 
	BMP_LONG	biYPelsPerMeter; 
	BMP_DWORD	biClrUsed; 
	BMP_DWORD	biClrImportant; 
} BMP_BITMAPINFOHEADER; 

class Image;
// global I/O routines
extern bool readBMP(const char *fname, Image& image);
//int& width, int& height, void* data_ptr);

#endif

