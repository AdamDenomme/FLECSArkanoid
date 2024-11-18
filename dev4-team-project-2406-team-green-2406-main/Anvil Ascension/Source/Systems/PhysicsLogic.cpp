#include "PhysicsLogic.h"
#include <algorithm> // For std::min and std::max
#include <cmath> // For std::sqrt

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

ESG::PhysicsLogic::AABB ESG::PhysicsLogic::ComputeAABB(const std::vector<GW::MATH::GVECTORF>& poly) {
    AABB aabb;
    aabb.min = poly[0];
    aabb.max = poly[0];

    for (const auto& vertex : poly) {
        aabb.min.x = std::min(aabb.min.x, vertex.x);
        aabb.min.y = std::min(aabb.min.y, vertex.y);
        aabb.min.z = std::min(aabb.min.z, vertex.z);
        aabb.max.x = std::max(aabb.max.x, vertex.x);
        aabb.max.y = std::max(aabb.max.y, vertex.y);
        aabb.max.z = std::max(aabb.max.z, vertex.z);
    }

    return aabb;
}

ESG::PhysicsLogic::Sphere ESG::PhysicsLogic::ComputeBoundingSphere(const std::vector<GW::MATH::GVECTORF>& poly) {
    Sphere sphere;
    GW::MATH::GVECTORF center = { 0, 0, 0 };
    for (const auto& vertex : poly) {
        GW::MATH::GVector::AddVectorF(center, vertex, center);
    }
    GW::MATH::GVector::ScaleF(center, 1.0f / static_cast<float>(poly.size()), center);
    sphere.center = center;

    float maxDistanceSquared = 0;
    for (const auto& vertex : poly) {
        GW::MATH::GVECTORF diff;
        GW::MATH::GVector::SubtractVectorF(vertex, center, diff);

        float distanceSquared;
        GW::GReturn result = GW::MATH::GVector::DotF(diff, diff, distanceSquared);
        if (result != GW::GReturn::SUCCESS) {
            // Handle the error appropriately, maybe set distanceSquared to 0 or log an error
            distanceSquared = 0;
        }

        if (distanceSquared > maxDistanceSquared) {
            maxDistanceSquared = distanceSquared;
        }
    }
    sphere.radius = std::sqrt(maxDistanceSquared);

    return sphere;
}

bool ESG::PhysicsLogic::TestAABBToAABB(const AABB& aabb1, const AABB& aabb2) {
    return (aabb1.min.x <= aabb2.max.x && aabb1.max.x >= aabb2.min.x) &&
        (aabb1.min.y <= aabb2.max.y && aabb1.max.y >= aabb2.min.y) &&
        (aabb1.min.z <= aabb2.max.z && aabb1.max.z >= aabb2.min.z);
}

bool ESG::PhysicsLogic::TestSphereToSphere(const Sphere& sphere1, const Sphere& sphere2) {
    GW::MATH::GVECTORF diff;
    GW::MATH::GVector::SubtractVectorF(sphere1.center, sphere2.center, diff);

    float distanceSquared;
    GW::GReturn result = GW::MATH::GVector::DotF(diff, diff, distanceSquared);
    if (result != GW::GReturn::SUCCESS) {
        return false; // Handle the error case appropriately
    }

    float radiusSum = sphere1.radius + sphere2.radius;
    return distanceSquared <= (radiusSum * radiusSum);
}

bool ESG::PhysicsLogic::TestAABBToSphere(const AABB& aabb, const Sphere& sphere) {
    float sqDist = 0.0f;
    float v[3] = { sphere.center.x, sphere.center.y, sphere.center.z }; // Extract components
    float aabbMin[3] = { aabb.min.x, aabb.min.y, aabb.min.z }; // Extract components
    float aabbMax[3] = { aabb.max.x, aabb.max.y, aabb.max.z }; // Extract components

    for (int i = 0; i < 3; ++i) {
        if (v[i] < aabbMin[i]) sqDist += (aabbMin[i] - v[i]) * (aabbMin[i] - v[i]);
        if (v[i] > aabbMax[i]) sqDist += (v[i] - aabbMax[i]) * (v[i] - aabbMax[i]);
    }

    return sqDist <= (sphere.radius * sphere.radius);
}

bool ESG::PhysicsLogic::Init(std::shared_ptr<flecs::world> _game, std::weak_ptr<const GameConfig> _gameConfig)
{
    game = _game;
    gameConfig = _gameConfig;

    game->system<Velocity, const Acceleration>("Acceleration System")
        .each([](flecs::entity e, Velocity& v, const Acceleration& a) {
        GW::MATH::GVECTORF accel;
    GW::MATH::GVector::ScaleF(a.value, e.delta_time(), accel);
    GW::MATH::GVector::AddVectorF(accel, v.value, v.value);
            });

    game->system<Position, const Velocity>("Translation System")
        .each([](flecs::entity e, Position& p, const Velocity& v) {
        GW::MATH::GVECTORF speed;
    GW::MATH::GVector::ScaleF(v.value, e.delta_time(), speed);
    GW::MATH::GVector::AddVectorF(speed, p.value, p.value);
            });

    game->system<const Position>("Cleanup System")
        .each([](flecs::entity e, const Position& p) {
        if (p.value.x > 1.5f || p.value.x < -1.5f ||
        p.value.y > 1.5f || p.value.y < -1.5f ||
            p.value.z > 1.5f || p.value.z < -1.5f) {
            e.destruct();
        }
            });

    queryCache = game->query<Collidable, Position, Orientation>();

    struct CollisionSystem {};
    game->entity("Detect-Collisions").add<CollisionSystem>();
    game->system<CollisionSystem>()
        .each([this](CollisionSystem& s) {
        constexpr GW::MATH::GVECTORF poly[polysize] = {
            { -0.5f, -0.5f, 0 }, { 0, 0.5f, 0 }, { 0.5f, -0.5f, 0 }, { 0, -0.25f, 0 }
            };

    queryCache.each([this, poly](flecs::entity e, Collidable& c, Position& p, Orientation& o) {
        GW::MATH::GMATRIXF matrix = GW::MATH::GIdentityMatrixF;
    GW::MATH::GMatrix::MultiplyMatrixF(matrix, o.value, matrix);
    matrix.row4 = { p.value.x, p.value.y, p.value.z, 1 };

    SHAPE polygon;
    polygon.owner = e;
    for (int i = 0; i < polysize; ++i) {
        GW::MATH::GVECTORF v = { poly[i].x, poly[i].y, poly[i].z, 1 };
        GW::MATH::GVector::TransformF(v, matrix, v);
        polygon.poly[i] = v;
    }
    testCache.push_back(polygon);
        });

    for (int i = 0; i < testCache.size(); ++i) {
        AABB aabb1 = ComputeAABB(std::vector<GW::MATH::GVECTORF>(std::begin(testCache[i].poly), std::end(testCache[i].poly)));
        Sphere sphere1 = ComputeBoundingSphere(std::vector<GW::MATH::GVECTORF>(std::begin(testCache[i].poly), std::end(testCache[i].poly)));

        for (int j = i + 1; j < testCache.size(); ++j) {
            AABB aabb2 = ComputeAABB(std::vector<GW::MATH::GVECTORF>(std::begin(testCache[j].poly), std::end(testCache[j].poly)));
            Sphere sphere2 = ComputeBoundingSphere(std::vector<GW::MATH::GVECTORF>(std::begin(testCache[j].poly), std::end(testCache[j].poly)));

            GW::MATH::GCollision::GCollisionCheck result = GW::MATH::GCollision::GCollisionCheck::NO_COLLISION;

            if (TestAABBToAABB(aabb1, aabb2)) {
                result = GW::MATH::GCollision::GCollisionCheck::COLLISION;
            }
            else if (TestSphereToSphere(sphere1, sphere2)) {
                result = GW::MATH::GCollision::GCollisionCheck::COLLISION;
            }
            else if (TestAABBToSphere(aabb1, sphere2) || TestAABBToSphere(aabb2, sphere1)) {
                result = GW::MATH::GCollision::GCollisionCheck::COLLISION;
            }

            if (result == GW::MATH::GCollision::GCollisionCheck::COLLISION) {
                testCache[j].owner.add<CollidedWith>(testCache[i].owner);
                testCache[i].owner.add<CollidedWith>(testCache[j].owner);
            }
        }
    }
    testCache.clear();
            });

    return true;
}

bool ESG::PhysicsLogic::Activate(bool runSystem)
{
    if (runSystem) {
        game->entity("Acceleration System").enable();
        game->entity("Translation System").enable();
        game->entity("Cleanup System").enable();
    }
    else {
        game->entity("Acceleration System").disable();
        game->entity("Translation System").disable();
        game->entity("Cleanup System").disable();
    }
    return true;
}

bool ESG::PhysicsLogic::Shutdown()
{
    queryCache.destruct();
    game->entity("Acceleration System").destruct();
    game->entity("Translation System").destruct();
    game->entity("Cleanup System").destruct();
    return true;
}