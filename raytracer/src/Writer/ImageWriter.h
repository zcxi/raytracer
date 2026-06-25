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
    bool autoExposure;
    double targetLuminance;

    ImageOutputSettings(double exposure = 0.0,
                        ToneMapper toneMapper = ToneMapper::Aces,
                        bool autoExposure = true,
                        double targetLuminance = 0.18)
        : exposure(exposure), toneMapper(toneMapper),
          autoExposure(autoExposure),
          targetLuminance(targetLuminance) {
    }
};

class ImageWriter {
public:
    explicit ImageWriter(
        const std::string& outputPath = "output.ppm",
        const ImageOutputSettings& settings = ImageOutputSettings())
        : outputPath(outputPath),
          settings(settings),
          configuredExposure(settings.exposure) {}

    bool write(const std::vector<std::vector<Vec3>>& image) const;
    bool writeAccumulation(
        const std::vector<std::vector<Vec3>>& accumulation,
        unsigned int completedSamples) const;
    bool writeAdaptive(
        const std::vector<std::vector<Vec3>>& averaged,
        unsigned int maxSamples) const;
    void setExposure(double value) { settings.exposure = value; }
    const ImageOutputSettings& getSettings() const { return settings; }
    double getConfiguredExposure() const { return configuredExposure; }
    static double applyExposure(double linearValue, double exposure);
    static double toneMap(double linearValue, ToneMapper toneMapper);
    static double linearToSrgb(double linearValue);
    static unsigned char toByte(
        double linearValue, double exposure = 0.0,
        ToneMapper toneMapper = ToneMapper::None);

private:
    bool writeScaled(
        const std::vector<std::vector<Vec3>>& image,
        double divisor) const;
    std::string outputPath;
    ImageOutputSettings settings;
    double configuredExposure;
};


#endif //RAYTRACER_IMAGEWRITER_H
