#include "Materials/Material.h"
#include "Math/Ray.h"
#include "Acceleration/SimdAabb.h"
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

#ifdef RAYTRACER_SIMD_ENABLED
        const int packetCount = 250000;
        const int totalRays = packetCount * 4;
        std::vector<std::vector<double>> originsX(
            packetCount, std::vector<double>(4));
        std::vector<std::vector<double>> originsY(
            packetCount, std::vector<double>(4));
        std::vector<std::vector<double>> originsZ(
            packetCount, std::vector<double>(4));
        std::vector<std::vector<double>> invDirX(
            packetCount, std::vector<double>(4));
        std::vector<std::vector<double>> invDirY(
            packetCount, std::vector<double>(4));
        std::vector<std::vector<double>> invDirZ(
            packetCount, std::vector<double>(4));
        std::vector<std::vector<double>> tMinData(
            packetCount, std::vector<double>(4, 0.0));
        std::vector<std::vector<double>> tMaxData(
            packetCount, std::vector<double>(4, 100.0));
        std::vector<Aabb> boxes;
        boxes.reserve(packetCount);
        for (int i = 0; i < packetCount; ++i) {
            double cx = (static_cast<double>(i * 7 % 100) - 50.0) * 0.1;
            double cy = (static_cast<double>(i * 13 % 100) - 50.0) * 0.1;
            double cz = (static_cast<double>(i * 17 % 100) - 50.0) * 0.1;
            boxes.emplace_back(
                Vec3(cx - 1.0, cy - 1.0, cz - 1.0),
                Vec3(cx + 1.0, cy + 1.0, cz + 1.0));
            for (int j = 0; j < 4; ++j) {
                double ox = (i * 4 + j) * 0.03 - 30.0;
                double oy = (i * 4 + j) * 0.07 - 30.0;
                double oz = (i * 4 + j) * 0.05 - 30.0;
                double dx = (i + j * 7) % 11 - 5.0;
                double dy = (i + j * 11) % 11 - 5.0;
                double dz = (i + j * 13) % 11 - 5.0;
                if (std::abs(dx) < 0.01) dx = 0.5;
                if (std::abs(dy) < 0.01) dy = 0.5;
                if (std::abs(dz) < 0.01) dz = 0.5;
                double length = std::sqrt(dx * dx + dy * dy + dz * dz);
                originsX[i][j] = ox;
                originsY[i][j] = oy;
                originsZ[i][j] = oz;
                invDirX[i][j] = 1.0 / (dx / length);
                invDirY[i][j] = 1.0 / (dy / length);
                invDirZ[i][j] = 1.0 / (dz / length);
            }
        }

        std::vector<bool> resultsScalar(totalRays, false);
        std::vector<bool> resultsSimd(totalRays, false);
        std::size_t hitsScalar = 0;
        std::size_t hitsSimd = 0;

        {
            const auto start = std::chrono::steady_clock::now();
            for (int i = 0; i < packetCount; ++i) {
                bool tmp[4] = {false, false, false, false};
                SimdAabb::intersectScalar(
                    boxes[i],
                    originsX[i].data(), originsY[i].data(),
                    originsZ[i].data(), invDirX[i].data(),
                    invDirY[i].data(), invDirZ[i].data(),
                    tMinData[i].data(), tMaxData[i].data(), tmp);
                for (int j = 0; j < 4; ++j) {
                    resultsScalar[i * 4 + j] = tmp[j];
                    if (tmp[j]) ++hitsScalar;
                }
            }
            const double secs = std::chrono::duration<double>(
                std::chrono::steady_clock::now() - start).count();
            std::cout << "simd_scalar," << std::fixed
                      << std::setprecision(6) << secs << ","
                      << std::setprecision(3)
                      << static_cast<double>(totalRays) / secs / 1e6
                      << "," << hitsScalar << "\n";
        }

        {
            const auto start = std::chrono::steady_clock::now();
            for (int i = 0; i < packetCount; ++i) {
                bool tmp[4] = {false, false, false, false};
                SimdAabb::intersect4(
                    boxes[i],
                    originsX[i].data(), originsY[i].data(),
                    originsZ[i].data(), invDirX[i].data(),
                    invDirY[i].data(), invDirZ[i].data(),
                    tMinData[i].data(), tMaxData[i].data(), tmp);
                for (int j = 0; j < 4; ++j) {
                    resultsSimd[i * 4 + j] = tmp[j];
                    if (tmp[j]) ++hitsSimd;
                }
            }
            const double secs = std::chrono::duration<double>(
                std::chrono::steady_clock::now() - start).count();
            std::cout << "simd_packet," << std::fixed
                      << std::setprecision(6) << secs << ","
                      << std::setprecision(3)
                      << static_cast<double>(totalRays) / secs / 1e6
                      << "," << hitsSimd << "\n";
        }

        bool allMatch = hitsScalar == hitsSimd;
        auto scalarIt = resultsScalar.begin();
        auto simdIt = resultsSimd.begin();
        for (; scalarIt != resultsScalar.end(); ++scalarIt, ++simdIt) {
            if (*scalarIt != *simdIt) {
                allMatch = false;
                break;
            }
        }
        std::cout << "simd_results_match," << (allMatch ? "yes" : "no") << "\n";
#endif

        std::remove(path.c_str());
        return accelerated.hits == brute.hits ? 0 : 1;
    } catch (const std::exception& error) {
        std::remove(path.c_str());
        std::cerr << "Benchmark failed: " << error.what() << "\n";
        return 1;
    }
}
