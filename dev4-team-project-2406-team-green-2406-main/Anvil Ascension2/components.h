#pragma once 

// define components
struct BlenderName { std::string name; };
struct Health { int health; };
struct Lives { int lives; };
struct Damage { int value; };
struct ModelBoundary {
    GW::MATH::GVECTORF minPoint;
    GW::MATH::GVECTORF maxPoint;

    ModelBoundary()
        : minPoint(GW::MATH::GVECTORF{ 0, 0, 0, 0 }), maxPoint(GW::MATH::GVECTORF{ 0, 0, 0, 0 }) {}

    ModelBoundary(const GW::MATH::GVECTORF& minVal, const GW::MATH::GVECTORF& maxVal)
        : minPoint(minVal), maxPoint(maxVal) {}
};
struct ModelTransform { 
	GW::MATH::GMATRIXF matrix;
	unsigned int rendererIndex;
};
struct Velocity {
	GW::MATH::GVECTORF value;
};
struct Power
{
	int powerUp;
};

// Individual TAGs
