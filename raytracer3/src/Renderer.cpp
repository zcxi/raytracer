//
// Created by chenx on 2020-05-25.
//

#include <math.h>
#include <cstdint>
#include <iostream>
#include <algorithm>
#include "Renderer.h"
#include "Math/Vec3.h"
#include "Math/Quaternion.h"

Renderer::Renderer(ImageWriter* imageWriter, Scene* scenePtr, Camera* camera){
    this->imageWriter = imageWriter;
    this->scene = scenePtr;
    this->camera = camera;
}

void Renderer::render(){

    int height = camera->getImageHeight();
    int width = camera->getImageWidth();

    double fov = camera->getFov();
    Vec3 cameraDirection = camera->GetDir();

    std::vector<std::vector<Vec3>> framebuffer(height, std::vector<Vec3>(width, Vec3()));
    double fovScale = tan(fov * 0.5);
    double aspectRatio = width / height;

    double x = -1, y = -1, z= -1;
    

    for (int j = 0; j < height; ++j) {

        y = (1 - 2 * (j + 0.5) / (double)height) * fovScale;

        for (int i = 0; i < width; ++i) {

            //map pixel coordinates to screen space
            x = (2 * (i + 0.5) / (double)width - 1) * aspectRatio * fovScale;

            Vec3 ray = (Vec3(x, y, z)).normalize();
            //ray = Quaternion::rotateCamera(ray, cameraDirection);
            Vec3 pixel = scene->trace(camera->GetPos(), ray, 0);
            

            framebuffer[j][i] = Vec3(std::min(MAX_COLOR_VALUE, (int)pixel.X()), std::min(MAX_COLOR_VALUE, (int)pixel.Y()), std::min(MAX_COLOR_VALUE, (int)pixel.Z()));
        }
    }
    imageWriter->write(framebuffer);

}
