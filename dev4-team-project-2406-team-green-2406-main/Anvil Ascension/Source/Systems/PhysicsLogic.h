#ifndef PHYSICSLOGIC_H
#define PHYSICSLOGIC_H

#include "../GameConfig.h"
#include "../Components/Physics.h"
#include <algorithm> // For std::min and std::max
#include <cmath> // For std::sqrt

namespace ESG
{
	class PhysicsLogic
	{
		std::shared_ptr<flecs::world> game;
		std::weak_ptr<const GameConfig> gameConfig;
		flecs::query<Collidable, Position, Orientation> queryCache;

		static constexpr unsigned polysize = 4;
		struct SHAPE {
			GW::MATH::GVECTORF poly[polysize];
			flecs::entity owner;
		};

		struct AABB {
			GW::MATH::GVECTORF min;
			GW::MATH::GVECTORF max;
		};

		struct Sphere {
			GW::MATH::GVECTORF center;
			float radius;
		};

		std::vector<SHAPE> testCache;

		AABB ComputeAABB(const std::vector<GW::MATH::GVECTORF>& poly);
		Sphere ComputeBoundingSphere(const std::vector<GW::MATH::GVECTORF>& poly);
		bool TestAABBToAABB(const AABB& aabb1, const AABB& aabb2);
		bool TestSphereToSphere(const Sphere& sphere1, const Sphere& sphere2);
		bool TestAABBToSphere(const AABB& aabb, const Sphere& sphere);

	public:
		bool Init(std::shared_ptr<flecs::world> _game, std::weak_ptr<const GameConfig> _gameConfig);
		bool Activate(bool runSystem);
		bool Shutdown();
	};
};

#endif