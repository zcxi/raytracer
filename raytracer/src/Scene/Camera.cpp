//
// Created by chenx on 2020-05-25.
//

#include "Camera.h"

#include <stdexcept>

Camera::Camera(const Vec3& position, const Vec3& rotationRadians,
               int width, int height, double verticalFovRadians)
    : pos(position),
      rotation(rotationRadians),
      imageWidth(width),
      imageHeight(height),
      fov(verticalFovRadians) {
    if (width <= 0 || height <= 0) {
        throw std::invalid_argument("Camera image dimensions must be positive.");
    }
    if (fov <= 0.0 || fov >= 3.14159265358979323846) {
        throw std::invalid_argument("Camera field of view must be between 0 and pi.");
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
    const double aspectRatio =
        static_cast<double>(imageWidth) / static_cast<double>(imageHeight);
    const double fovScale = std::tan(fov * 0.5);
    const double screenX =
        (2.0 * pixelX / static_cast<double>(imageWidth) - 1.0) *
        aspectRatio * fovScale;
    const double screenY =
        (1.0 - 2.0 * pixelY / static_cast<double>(imageHeight)) * fovScale;

    return Ray(pos, forward + right * screenX + up * screenY);
}
