//
// Created by chenx on 2020-05-25.
//

#ifndef RAYTRACER_IMAGEWRITER_H
#define RAYTRACER_IMAGEWRITER_H


#include "../Math/Vec3.h"

class ImageWriter {
public:
    static bool write(std::vector<std::vector<Vec3>> &image);
};


#endif //RAYTRACER_IMAGEWRITER_H
