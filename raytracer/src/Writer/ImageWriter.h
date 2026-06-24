//
// Created by chenx on 2020-05-25.
//

#ifndef RAYTRACER_IMAGEWRITER_H
#define RAYTRACER_IMAGEWRITER_H


#include "../Math/Vec3.h"
#include <string>
#include <vector>

enum class ToneMapper {
    None,
    Reinhard,
    Aces
};

struct ImageOutputSettings {
    double exposure;
    ToneMapper toneMapper;

    ImageOutputSettings(double exposure = 0.0,
                        ToneMapper toneMapper = ToneMapper::Aces)
        : exposure(exposure), toneMapper(toneMapper) {
    }
};

class ImageWriter {
public:
    explicit ImageWriter(
        const std::string& outputPath = "output.ppm",
        const ImageOutputSettings& settings = ImageOutputSettings())
        : outputPath(outputPath), settings(settings) {}

    bool write(const std::vector<std::vector<Vec3>>& image) const;
    static double applyExposure(double linearValue, double exposure);
    static double toneMap(double linearValue, ToneMapper toneMapper);
    static double linearToSrgb(double linearValue);
    static unsigned char toByte(
        double linearValue, double exposure = 0.0,
        ToneMapper toneMapper = ToneMapper::None);

private:
    std::string outputPath;
    ImageOutputSettings settings;
};


#endif //RAYTRACER_IMAGEWRITER_H
