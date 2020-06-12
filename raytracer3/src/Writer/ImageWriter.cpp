//
// Created by chenx on 2020-05-25.
//

#include <fstream>
#include <iostream>
#include "ImageWriter.h"


bool ImageWriter::write(std::vector<std::vector<Vec3>> &image){

    uint32_t width = image[0].size();
    uint32_t height = image.size();

    std::ofstream ofs("C:/Users/chenx/Documents/projects/raytracer/untitled.ppm", std::ios::out | std::ios::binary);
    ofs << "P6\n" << width << " " << height << "\n255\n";

    // outputs color of each pixel
    for (uint32_t i = 0; i < height; i++) {
        for (uint32_t j = 0; j < width; j++) {

            ofs << (unsigned char)(image[i][j].X()) <<
                (unsigned char)(image[i][j].Y()) <<
                (unsigned char)(image[i][j].Z());
        }
    }

    ofs.close();
    return true;
}
