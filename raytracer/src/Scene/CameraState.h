#ifndef RAYTRACER_CAMERA_STATE_H
#define RAYTRACER_CAMERA_STATE_H

#include "Camera.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <stdexcept>

struct CameraState {
    Vec3 position;
    Vec3 rotation;
    int imageWidth;
    int imageHeight;
    double verticalFov;
    double aperture;
    double focusDistance;
    double shutterOpen;
    double shutterClose;

    CameraState()
        : position(), rotation(), imageWidth(1), imageHeight(1),
          verticalFov(1.0), aperture(0.0), focusDistance(1.0),
          shutterOpen(0.0), shutterClose(0.0) {
    }

    CameraState(const Vec3& position, const Vec3& rotation,
                int imageWidth, int imageHeight, double verticalFov,
                double aperture = 0.0, double focusDistance = 1.0,
                double shutterOpen = 0.0, double shutterClose = 0.0)
        : position(position), rotation(rotation),
          imageWidth(imageWidth), imageHeight(imageHeight),
          verticalFov(verticalFov), aperture(aperture),
          focusDistance(focusDistance),
          shutterOpen(shutterOpen), shutterClose(shutterClose) {
    }

    static CameraState fromCamera(const Camera& camera) {
        return CameraState(
            camera.getPosition(), camera.getRotation(),
            camera.getImageWidth(), camera.getImageHeight(),
            camera.getFov(), camera.getAperture(),
            camera.getFocusDistance(), camera.getShutterOpen(),
            camera.getShutterClose());
    }

    std::unique_ptr<Camera> makeCamera() const {
        return std::unique_ptr<Camera>(new Camera(
            position, rotation, imageWidth, imageHeight, verticalFov,
            aperture, focusDistance, shutterOpen, shutterClose));
    }

    CameraState withImageScale(double scale) const {
        if (!std::isfinite(scale) || scale <= 0.0) {
            throw std::invalid_argument(
                "Preview scale must be a finite positive number.");
        }
        CameraState scaled = *this;
        scaled.imageWidth = std::max(
            1, static_cast<int>(std::round(imageWidth * scale)));
        scaled.imageHeight = std::max(
            1, static_cast<int>(std::round(imageHeight * scale)));
        return scaled;
    }
};

#endif
