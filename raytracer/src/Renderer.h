//
// Created by chenx on 2020-05-25.
//

#ifndef RAYTRACER_RENDERER_H
#define RAYTRACER_RENDERER_H


#include "Writer/ImageWriter.h"
#include "Scene/Scene.h"
#include <mutex>

class Renderer {

    const int MAX_COLOR_VALUE = 255;

    public:
        Renderer(ImageWriter* imageWriter, Scene* scenePtr, Camera* camera);

        void render();


    private:
        ImageWriter* imageWriter;
        Scene* scene;
        Camera* camera;

        std::vector<Vec3> rays;
        std::vector<std::pair<int, int>> rayCoords;
        std::mutex rays_mutex;
        std::mutex frame_mutex;
        std::vector<std::vector<Vec3>> frameBuffer;
        static void traceJob(Renderer* renderer);
};


#endif //RAYTRACER_RENDERER_H
