#include <iostream>
#include "./Scene/Scene.h"
#include "./shapes/Sphere.h"
#include "Writer/ImageWriter.h"
#include "./Scene/Light/PointSource.h"
#include "Renderer.h"
#include "shapes/Plane.h"

#define _USE_MATH_DEFINES
#include <math.h>

void demo1 () {
    Scene* s = new Scene();

    Sphere* sph1 =  new Sphere(Vec3(6, -8, -60), 4, Vec3(255, 0, 0), Vec3(), 1, 0);
    Sphere* sph2 = new Sphere(Vec3(6, 8, -60), 4, Vec3(255, 0, 100), Vec3(), 1, 0);
    Sphere* sph3 = new Sphere(Vec3(-6, 8, -60), 4, Vec3(255, 50, 50), Vec3(), 1, 0);
    Sphere* sph4 = new Sphere(Vec3(-6, -8, -60), 4, Vec3(255, 50, 0), Vec3(), 1, 0);
    Plane* plane1 = new Plane(Vec3(0, 0, -2), Vec3(0, 0, -64), Vec3(255, 0, 255), Vec3(), 0, 0);
    std::vector<Shape*> shapes;
    shapes.push_back(sph1);
    shapes.push_back(sph2);
    shapes.push_back(sph3);
    shapes.push_back(sph4);
    shapes.push_back(plane1);

    s->addShapes(shapes);

    PointSource* light = new PointSource(Vec3(40, 40, 10), Vec3(255, 255, 255), 250000);

    std::vector<LightSource*> lights;
    lights.push_back(light);
    s->addLights(lights);

    Camera* camera = new Camera(Vec3(0, 0.1, 0), Vec3(M_PI/12, 0, 0), 640, 640, M_PI / 6);
    ImageWriter* imageWriter = new ImageWriter();
    Renderer render(imageWriter, s, camera);
    render.render();

}

void demo2(){


}

int main() {

    demo1();
    return 0;
}