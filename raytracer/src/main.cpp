#include <iostream>
#include <memory>
#include "Scene/Scene.h"
#include "Shapes/Sphere.h"
#include "Writer/ImageWriter.h"
#include "Scene/Light/PointSource.h"
#include "Renderer.h"
#include "Shapes/Cube.h"
#include "Shapes/Plane.h"
#include "Shapes/Pyramid.h"
#include "Shapes/RectangularPrism.h"

void demo1(const std::string& outputPath) {
    const double pi = 3.14159265358979323846;
    Scene scene(Vec3(0.02, 0.03, 0.05));

    scene.addShape(std::unique_ptr<Shape>(
        new Cube(Vec3(-7, 7, -58), 7.0,
                 Vec3(0.15, 0.65, 1.0), Vec3(), 0.0, 1.5)));
    scene.addShape(std::unique_ptr<Shape>(
        new RectangularPrism(
            Vec3(7, 7, -58), Vec3(10, 6, 5),
            Vec3(0.2, 1.0, 0.35), Vec3(), 0.0, 1.5)));
    scene.addShape(std::unique_ptr<Shape>(
        new Sphere(Vec3(-7, -7, -58), 4,
                   Vec3(1.0, 0.2, 0.1), Vec3(), 0.0, 1.5)));
    scene.addShape(std::unique_ptr<Shape>(
        new Pyramid(Vec3(7, -11, -59), 10, 7, 10,
                    Vec3(1.0, 0.75, 0.1), Vec3(), 0.0, 1.5)));
    scene.addShape(std::unique_ptr<Shape>(
        new Plane(Vec3(0, 0, -1), Vec3(0, 0, -64),
                  Vec3(1.0, 0.0, 1.0), Vec3(), 0.0, 1.0)));

    scene.addLight(std::unique_ptr<LightSource>(
        new PointSource(Vec3(40, 40, 10), Vec3(1.0, 1.0, 1.0), 150000)));

    Camera camera(Vec3(0, 0.1, 0), Vec3(),
                  640, 640, pi / 6);
    ImageWriter imageWriter(outputPath);
    Renderer render(imageWriter, scene, camera);
    render.render();
}

int main(int argc, char* argv[]) {
    const std::string outputPath = argc > 1 ? argv[1] : "output.ppm";
    demo1(outputPath);
    std::cout << "Wrote " << outputPath << std::endl;
    return 0;
}
