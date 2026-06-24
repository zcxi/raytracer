//
// Created by chenx on 2020-05-25.
//

#ifndef RAYTRACER_IMAGEWRITER_H
#define RAYTRACER_IMAGEWRITER_H


#include "../Math/Vec3.h"
#include <string>
#include <vector>

class ImageWriter {
public:
    explicit ImageWriter(const std::string& outputPath = "output.ppm")
        : outputPath(outputPath) {}

    bool write(const std::vector<std::vector<Vec3>>& image) const;
    static double linearToSrgb(double linearValue);
    static unsigned char toByte(double linearValue);

private:
    std::string outputPath;
};


#endif //RAYTRACER_IMAGEWRITER_H
