// define all ECS components related to drawing
#ifndef VISUALS_H
#define VISUALS_H

// example space game (avoid name collisions)
namespace ESG
{
	struct Color { GW::MATH::GVECTORF value; };

	struct Material {
		Color diffuse = { 1, 1, 1 };
	};

	struct Model {
		std::string path;
		Model(const std::string& modelPath = "") : path(modelPath) {}
	};
};

#endif