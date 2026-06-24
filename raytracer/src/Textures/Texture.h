#ifndef RAYTRACER_TEXTURE_H
#define RAYTRACER_TEXTURE_H

#include "../Math/Vec3.h"
#include <memory>
#include <string>
#include <vector>

class Texture {
public:
    virtual ~Texture() {}
    virtual Vec3 value(double u, double v, const Vec3& point) const = 0;
};

class SolidTexture : public Texture {
public:
    explicit SolidTexture(const Vec3& color) : color(color) {}
    Vec3 value(double u, double v, const Vec3& point) const override;
private:
    Vec3 color;
};

class CheckerTexture : public Texture {
public:
    CheckerTexture(const Vec3& first, const Vec3& second, double scale);
    Vec3 value(double u, double v, const Vec3& point) const override;
private:
    Vec3 first;
    Vec3 second;
    double scale;
};

class ImageTexture : public Texture {
public:
    ImageTexture();
    bool loadPpm(const std::string& path);
    Vec3 value(double u, double v, const Vec3& point) const override;
private:
    std::vector<Vec3> pixels;
    unsigned int width;
    unsigned int height;
};

#endif

