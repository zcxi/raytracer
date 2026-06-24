//
// Created by chenx on 2020-05-25.
//

#ifndef RAYTRACER_CAMERA_H
#define RAYTRACER_CAMERA_H


#include <cmath>
#include "../Math/Vec3.h"
#include "../Math/Ray.h"

class Camera {

     public:
        Camera(const Vec3& pos, const Vec3& rotationRadians,
               int width, int height, double verticalFovRadians);

        const Vec3& getPosition() const { return pos; }
        const Vec3& getRotation() const { return rotation; }
        const Vec3& getForward() const { return forward; }
        const Vec3& getRight() const { return right; }
        const Vec3& getUp() const { return up; }
        int getImageWidth() const { return imageWidth; }
        int getImageHeight() const { return imageHeight; }
        double getFov() const { return fov; }
        Ray makeRay(double pixelX, double pixelY) const;


    private:
        Vec3 pos;
        Vec3 rotation;
        Vec3 forward;
        Vec3 right;
        Vec3 up;
        int imageWidth;
        int imageHeight;
        double fov;
};


#endif //RAYTRACER_CAMERA_H
