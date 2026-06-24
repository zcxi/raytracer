#include "ObjMesh.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace {

struct FaceIndex {
    int position;
    int texcoord;
    int normal;
};

FaceIndex parseIndex(const std::string& text) {
    FaceIndex result = {0, 0, 0};
    std::stringstream stream(text);
    std::string part;
    if (std::getline(stream, part, '/')) result.position = std::stoi(part);
    if (std::getline(stream, part, '/') && !part.empty())
        result.texcoord = std::stoi(part);
    if (std::getline(stream, part, '/') && !part.empty())
        result.normal = std::stoi(part);
    return result;
}

} // namespace

ObjMesh::ObjMesh(const std::string& path, const Material& material)
    : Shape(material), triangles(), bounds() {
    std::ifstream input(path.c_str());
    if (!input) throw std::runtime_error("Failed to open OBJ: " + path);
    std::vector<Vec3> positions(1);
    std::vector<Vec3> normals(1);
    std::vector<Vec3> texcoords(1);
    std::string line;
    while (std::getline(input, line)) {
        std::stringstream stream(line);
        std::string type;
        stream >> type;
        if (type == "v") {
            double x, y, z; stream >> x >> y >> z;
            positions.push_back(Vec3(x, y, z));
        } else if (type == "vn") {
            double x, y, z; stream >> x >> y >> z;
            normals.push_back(Vec3(x, y, z));
        } else if (type == "vt") {
            double u, v; stream >> u >> v;
            texcoords.push_back(Vec3(u, v, 0.0));
        } else if (type == "f") {
            std::vector<FaceIndex> face;
            std::string token;
            while (stream >> token) face.push_back(parseIndex(token));
            for (std::size_t i = 1; i + 1 < face.size(); ++i) {
                const FaceIndex ids[3] = {face[0], face[i], face[i + 1]};
                Vertex vertices[3];
                for (int j = 0; j < 3; ++j) {
                    vertices[j].position = positions.at(ids[j].position);
                    if (ids[j].normal) vertices[j].normal =
                        normals.at(ids[j].normal);
                    if (ids[j].texcoord) {
                        const Vec3 uv = texcoords.at(ids[j].texcoord);
                        vertices[j].u = uv.X(); vertices[j].v = uv.Y();
                    }
                }
                triangles.push_back(
                    Triangle(vertices[0], vertices[1], vertices[2], material));
            }
        }
    }
    if (triangles.empty())
        throw std::runtime_error("OBJ contains no triangles: " + path);
    triangles[0].boundingBox(bounds);
    for (std::size_t i = 1; i < triangles.size(); ++i) {
        Aabb triangleBounds;
        triangles[i].boundingBox(triangleBounds);
        bounds = Aabb::surrounding(bounds, triangleBounds);
    }
}

bool ObjMesh::intersect(const Ray& ray, double minDistance,
                        double maxDistance, HitRecord& hit) const {
    bool found = false;
    double closest = maxDistance;
    for (const Triangle& triangle : triangles) {
        HitRecord candidate;
        if (triangle.intersect(ray, minDistance, closest, candidate)) {
            closest = candidate.distance;
            hit = candidate;
            hit.shape = this;
            found = true;
        }
    }
    return found;
}

bool ObjMesh::boundingBox(Aabb& output) const {
    output = bounds;
    return true;
}

