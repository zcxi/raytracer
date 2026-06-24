//
// Created by chenx on 2020-05-25.
//

#include "Camera.h"

#include <stdexcept>

Camera::Camera(const Vec3& position, const Vec3& rotationRadians,
               int width, int height, double verticalFovRadians,
               double aperture, double focusDistance,
               double shutterOpen, double shutterClose)
    : pos(position),
      rotation(rotationRadians),
      imageWidth(width),
      imageHeight(height),
      fov(verticalFovRadians),
      aperture(aperture),
      focusDistance(focusDistance),
      shutterOpen(shutterOpen),
      shutterClose(shutterClose) {
    if (width <= 0 || height <= 0) {
        throw std::invalid_argument("Camera image dimensions must be positive.");
    }
    if (fov <= 0.0 || fov >= 3.14159265358979323846) {
        throw std::invalid_argument("Camera field of view must be between 0 and pi.");
    }
    if (aperture < 0.0 || focusDistance <= 0.0 ||
        shutterClose < shutterOpen) {
        throw std::invalid_argument("Invalid lens or shutter settings.");
    }

    const double pitch = rotation.X();
    const double yaw = rotation.Y();
    const double roll = rotation.Z();

    forward = Vec3(std::sin(yaw) * std::cos(pitch),
                   std::sin(pitch),
                   -std::cos(yaw) * std::cos(pitch)).normalize();

    const Vec3 worldUp(0.0, 1.0, 0.0);
    Vec3 baseRight = forward.cross(worldUp).normalize();
    if (baseRight.getLength() <= Vec3::EPSILON) {
        baseRight = Vec3(1.0, 0.0, 0.0);
    }
    const Vec3 baseUp = baseRight.cross(forward).normalize();

    right = (baseRight * std::cos(roll) + baseUp * std::sin(roll)).normalize();
    up = (baseUp * std::cos(roll) - baseRight * std::sin(roll)).normalize();
}

Ray Camera::makeRay(double pixelX, double pixelY) const {
    Sampler sampler(0, 0, 1);
    return makeRay(pixelX, pixelY, sampler);
}

Ray Camera::makeRay(
        double pixelX, double pixelY, Sampler& sampler) const {
    const double aspectRatio =
        static_cast<double>(imageWidth) / static_cast<double>(imageHeight);
    const double fovScale = std::tan(fov * 0.5);
    const double screenX =
        (2.0 * pixelX / static_cast<double>(imageWidth) - 1.0) *
        aspectRatio * fovScale;
    const double screenY =
        (1.0 - 2.0 * pixelY / static_cast<double>(imageHeight)) * fovScale;

    const Vec3 pinholeDirection =
        (forward + right * screenX + up * screenY).normalize();
    Vec3 origin = pos;
    Vec3 target = pos + pinholeDirection * focusDistance;
    if (aperture > 0.0) {
        const double radius = std::sqrt(sampler.next()) * aperture * 0.5;
        const double angle =
            sampler.next() * 2.0 * 3.14159265358979323846;
        origin = origin + right * (radius * std::cos(angle)) +
                 up * (radius * std::sin(angle));
    }
    const double time = shutterOpen +
        (shutterClose - shutterOpen) * sampler.next();
    return Ray(origin, target - origin, time);
}
