//
// Created by chenx on 2020-05-25.
//

#ifndef RAYTRACER_RENDERER_H
#define RAYTRACER_RENDERER_H


#include "Writer/ImageWriter.h"
#include "Scene/Scene.h"
#include "Scene/Camera.h"
#include <atomic>

class Renderer {

    public:
        Renderer(ImageWriter& imageWriter, const Scene& scene, const Camera& camera);

        void render();


    private:
        ImageWriter& imageWriter;
        const Scene& scene;
        const Camera& camera;
        std::vector<std::vector<Vec3>> frameBuffer;
};


#endif //RAYTRACER_RENDERER_H
