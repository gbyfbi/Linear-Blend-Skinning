#ifndef JPEGIO_H
#define JPEGIO_H

#include <string>
#include "image.h"

bool SaveJPEG(const std::string& filename,
              int image_width,
              int image_height,
              const unsigned char* pixels);
bool LoadJPEG(const std::string& file_name, Image* image);

#endif
