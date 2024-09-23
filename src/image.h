#ifndef IMAGE_IMAGE
#define IMAGE_IMAGE

#define STBI_ONLY_PNG
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <stdint.h>
#include <vector>

struct Image{
    size_t width;
    size_t height;
    std::vector<uint8_t>* pixels;

    // ~Image(){
    //     if(pixels){
    //         delete pixels;
    //     }
    // }
};


#ifdef DEBUG
#define getImagePixels(data) getImagePixelsInner(data)
#else
#define getImagePixels(data) getImagePixelsInner(data, data##_LEN)
#endif

Image getImagePixelsInner
#ifndef DEBUG
(unsigned char* data, unsigned int len){
    int width, height;
    unsigned char* pixels = stbi_load_from_memory(data,len,&width,&height,nullptr,4);
#else
(const char* filename){
    int width, height;
    unsigned char* pixels = stbi_load(filename, &width, &height, nullptr, 4);
#endif
    std::vector<uint8_t>* out = new std::vector<uint8_t>();
    out->resize(width*height*4);
    memcpy(out->data(),pixels,width*height*4);
    stbi_image_free(pixels);
    return {
        (size_t)width,
        (size_t)height,
        out
    };
}


#endif