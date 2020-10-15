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
#include <thread>

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

    this->frameBuffer = std::vector<std::vector<Vec3>>(height, std::vector<Vec3>(width, Vec3()));
    double fovScale = tan(fov * 0.5);
    double aspectRatio = width / height;

    double x = -1, y = -1, z= -1;
    
    for (int j = 0; j < height; ++j) {


        for (int i = 0; i < width; ++i) {  
            


            y = (1 - 2 * (j + 0.5) / (double)height) * fovScale;

            //map pixel coordinates to screen space
            x = (2 * (i + 0.5) / (double)width - 1) * aspectRatio * fovScale;

            Vec3 ray = Vec3(x, y, z);
            
            this->rays.push_back(ray);
            this->rayCoords.push_back(std::make_pair(j, i));
            
        }
    }

    int numThreads = std::thread::hardware_concurrency();
    std::vector<std::thread> pool;

    for (int i = 0; i < numThreads; i++)
    {   
        pool.push_back( std::thread(traceJob, this));
        
    }
    for (int i = 0; i < numThreads; i++)
    {
        pool[i].join();

    }


    imageWriter->write(this->frameBuffer);

}

void Renderer::traceJob(Renderer * renderer) {
    
    while (true) {
        renderer->rays_mutex.lock();
        if (renderer->rays.size() == 0) {
            
            renderer->rays_mutex.unlock();
            break;
        }
        Vec3 ray = renderer->rays.back();
        renderer->rays.pop_back();

        int j = renderer->rayCoords.back().first;
        int i = renderer->rayCoords.back().second;
        renderer->rayCoords.pop_back();

        renderer->rays_mutex.unlock();

        ray = ray.normalize();
        Vec3 pixel = renderer->scene->trace(renderer->camera->GetPos(), ray, 0);
        renderer->frame_mutex.lock();
        renderer->frameBuffer[j][i] = Vec3(std::min(renderer->MAX_COLOR_VALUE, (int)pixel.X()), std::min(renderer->MAX_COLOR_VALUE, (int)pixel.Y()), std::min(renderer->MAX_COLOR_VALUE, (int)pixel.Z()));
        renderer->frame_mutex.unlock();

    }

}
