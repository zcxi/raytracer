//
// Created by chenx on 2020-05-25.
//

#ifndef RAYTRACER_RENDERER_H
#define RAYTRACER_RENDERER_H


#include "Writer/ImageWriter.h"
#include "Scene/Scene.h"

class Renderer {

    const int MAX_COLOR_VALUE = 255;

    public:
        Renderer(ImageWriter* imageWriter, Scene* scenePtr, Camera* camera);

        void render();


    private:
        ImageWriter* imageWriter;
        Scene* scene;
        Camera* camera;
};


#endif //RAYTRACER_RENDERER_H
