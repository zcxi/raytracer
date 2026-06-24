#include "Materials/Material.h"
#include "Math/Ray.h"
#include "Shapes/HitRecord.h"
#include "Shapes/ObjMesh.h"

#include <chrono>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace {

struct Result {
    double seconds;
    std::size_t hits;
};

void writeGrid(const std::string& path, int cells) {
    std::ofstream output(path.c_str());
    for (int y = 0; y <= cells; ++y) {
        for (int x = 0; x <= cells; ++x) {
            output << "v "
                   << static_cast<double>(x) / cells * 20.0 - 10.0
                   << " "
                   << static_cast<double>(y) / cells * 20.0 - 10.0
                   << " -10\n";
        }
    }
    for (int y = 0; y < cells; ++y) {
        for (int x = 0; x < cells; ++x) {
            const int first = y * (cells + 1) + x + 1;
            const int second = first + 1;
            const int fourth = first + cells + 1;
            const int third = fourth + 1;
            output << "f " << first << " " << second
                   << " " << third << " " << fourth << "\n";
        }
    }
}

template <typename Intersector>
Result measure(const std::vector<Ray>& rays, Intersector intersect) {
    const std::chrono::steady_clock::time_point start =
        std::chrono::steady_clock::now();
    std::size_t hits = 0;
    for (const Ray& ray : rays) {
        HitRecord hit;
        if (intersect(ray, hit)) {
            ++hits;
        }
    }
    const std::chrono::duration<double> elapsed =
        std::chrono::steady_clock::now() - start;
    Result result = {elapsed.count(), hits};
    return result;
}

void printResult(const std::string& name, const Result& result,
                 std::size_t rayCount) {
    std::cout << name << ","
              << std::fixed << std::setprecision(6) << result.seconds << ","
              << std::setprecision(3)
              << static_cast<double>(rayCount) /
                     result.seconds / 1e6 << ","
              << result.hits << "\n";
}

} // namespace

int main() {
    const std::string path = "benchmark-grid.obj";
    const int cells = 50;
    writeGrid(path, cells);

    try {
        ObjMesh mesh(
            path, Material::diffuse(Vec3(0.5, 0.5, 0.5)));
        std::vector<Ray> rays;
        const int rayDimension = 100;
        rays.reserve(rayDimension * rayDimension);
        for (int y = 0; y < rayDimension; ++y) {
            for (int x = 0; x < rayDimension; ++x) {
                rays.push_back(Ray(
                    Vec3(
                        (x + 0.5) / rayDimension * 20.0 - 10.0,
                        (y + 0.5) / rayDimension * 20.0 - 10.0,
                        0.0),
                    Vec3(0.0, 0.0, -1.0)));
            }
        }

        const Result accelerated = measure(
            rays, [&mesh](const Ray& ray, HitRecord& hit) {
                return mesh.intersect(ray, 1e-4, 100.0, hit);
            });
        const Result brute = measure(
            rays, [&mesh](const Ray& ray, HitRecord& hit) {
                return mesh.intersectBruteForce(
                    ray, 1e-4, 100.0, hit);
            });

        std::cout << "mode,seconds,mrays_per_second,hits\n";
        printResult("mesh_bvh", accelerated, rays.size());
        printResult("mesh_brute", brute, rays.size());
        std::cout << "mesh_speedup,"
                  << std::fixed << std::setprecision(2)
                  << brute.seconds / accelerated.seconds << "\n";
        std::remove(path.c_str());
        return accelerated.hits == brute.hits ? 0 : 1;
    } catch (const std::exception& error) {
        std::remove(path.c_str());
        std::cerr << "Benchmark failed: " << error.what() << "\n";
        return 1;
    }
}
