//
// Created by chenx on 2020-05-25.
//

#ifndef RAYTRACER_CAMERA_H
#define RAYTRACER_CAMERA_H


#include <math.h>
#include "../Math/Vec3.h"

class Camera {

     public:
        Camera(Vec3 pos, Vec3 dir, int width, int height, double fov) :
                pos(pos), dir(dir), imageWidth(width), imageHeight(height), fov(fov){
        }

        Vec3 GetPos(){
            return pos;
        }

        Vec3 GetDir(){
            return dir;
        }

        int getImageWidth(){
            return imageWidth;
        }

        int getImageHeight(){
            return imageHeight;
        }

        double getFov(){
            return fov;
        }


    private:
        Vec3 pos;
        Vec3 dir;
        int imageWidth;
        int imageHeight;
        double fov;
};


#endif //RAYTRACER_CAMERA_H
