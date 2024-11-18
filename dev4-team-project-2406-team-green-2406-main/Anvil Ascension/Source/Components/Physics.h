// define all ECS components related to movement & collision
#ifndef PHYSICS_H
#define PHYSICS_H

// example space game (avoid name collisions)
namespace ESG 
{
	// ECS component types should be *strongly* typed for proper queries
	// typedef is tempting but it does not help templates/functions resolve type
	struct Position { GW::MATH::GVECTORF value; }; // 3D vector
	struct Velocity { GW::MATH::GVECTORF value; }; // 3D vector
	struct Orientation { GW::MATH::GMATRIXF value; }; // 3D matrix
	struct Acceleration { GW::MATH::GVECTORF value; }; // 3D vector

	// Individual TAGs
	struct Collidable {}; 
	
	// ECS Relationship tags
	struct CollidedWith {};
};

#endif