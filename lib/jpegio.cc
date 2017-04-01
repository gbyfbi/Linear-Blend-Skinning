#include "jpegio.h"
#include <vector>
#include <jpeglib.h>
#include <stdio.h>

bool SaveJPEG(const std::string& filename,
              int image_width,
              int image_height,
              const unsigned char* pixels)
{
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	FILE* outfile;
	JSAMPROW row_pointer[1];
	int row_stride;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);

	outfile = fopen(filename.c_str(), "wb");
	if (outfile == NULL)
		return false;

	jpeg_stdio_dest(&cinfo, outfile);

	cinfo.image_width = image_width;
	cinfo.image_height = image_height;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;
	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, 100, true);
	jpeg_start_compress(&cinfo, true);

	row_stride = image_width * 3;

	while (cinfo.next_scanline < cinfo.image_height) {
		row_pointer[0] = const_cast<unsigned char*>(
				&pixels[(cinfo.image_height - 1 - cinfo.next_scanline) * row_stride]);
		jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}

	jpeg_finish_compress(&cinfo);
	fclose(outfile);

	jpeg_destroy_compress(&cinfo);
	return true;
}

bool LoadJPEG(const std::string& file_name, Image* image)
{
	FILE* file = fopen(file_name.c_str(), "rb");
	struct jpeg_decompress_struct info;
	struct jpeg_error_mgr err;

	info.err = jpeg_std_error(&err);
	jpeg_create_decompress(&info);

	if (file == NULL)
		return false;

	jpeg_stdio_src(&info, file);
	jpeg_read_header(&info, true);
	jpeg_start_decompress(&info);

	image->width = info.output_width;
	image->height = info.output_height;

	int channels = info.num_components;
	long size = image->width * image->height * 3;

	image->bytes.resize(size);

	int a = (channels > 2 ? 1 : 0);
	int b = (channels > 2 ? 2 : 0);
	std::vector<unsigned char> scan_line(image->width * channels, 0);
	unsigned char* p1 = &scan_line[0];
	unsigned char** p2 = &p1;
	unsigned char* out_scan_line = image->bytes.data();
	while (info.output_scanline < info.output_height) {
		jpeg_read_scanlines(&info, p2, 1);
		for (int i = 0; i < image->width; ++i) {
			out_scan_line[3 * i] = scan_line[channels * i];
			out_scan_line[3 * i + 1] = scan_line[channels * i + a];
			out_scan_line[3 * i + 2] = scan_line[channels * i + b];
		}
		out_scan_line += image->width * 3;
	}
	jpeg_finish_decompress(&info);
	fclose(file);
	return true;
}

