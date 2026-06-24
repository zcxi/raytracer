#ifndef RAYTRACER_TRACE_STATS_H
#define RAYTRACER_TRACE_STATS_H

#include <cstdint>

struct TraceStats {
    std::uint64_t rays;
    std::uint64_t shadowRays;
    std::uint64_t aabbTests;
    std::uint64_t primitiveTests;
    std::uint64_t bvhNodeVisits;
    std::uint64_t paths;
    std::uint64_t pathVertices;

    TraceStats()
        : rays(0),
          shadowRays(0),
          aabbTests(0),
          primitiveTests(0),
          bvhNodeVisits(0),
          paths(0),
          pathVertices(0) {
    }

    TraceStats& operator+=(const TraceStats& other) {
        rays += other.rays;
        shadowRays += other.shadowRays;
        aabbTests += other.aabbTests;
        primitiveTests += other.primitiveTests;
        bvhNodeVisits += other.bvhNodeVisits;
        paths += other.paths;
        pathVertices += other.pathVertices;
        return *this;
    }

    double averagePathDepth() const {
        return paths == 0
            ? 0.0
            : static_cast<double>(pathVertices) /
                  static_cast<double>(paths);
    }
};

TraceStats* currentTraceStats();

class TraceStatsScope {
public:
    explicit TraceStatsScope(TraceStats* stats);
    ~TraceStatsScope();

private:
    TraceStats* previous;
};

#endif
