// A simple "game" where you use the mouse to move around objects to test collisions.
// In this game the shapes are 2D but exist in a 3D world with a camera/projection.
// Unlike Example Space Game, this is designed with EnTT and zero OOP code.
// It is also organized in a very simple and direct manner. (Data Oriented Design)

// The actual code for the game:
namespace ESG { // avoid name collisions

	// Defines the data used in the game
	#include "components.h"
	// Creates the data used in the game
	#include "entities.h"
	// Processes the data used in the game
	#include "systems.h"

} // end namespace

// main game loop
int main(void) {
	if (ESG::CreateGameLoop())
		return ESG::ProcessGameLoop();
	return 1; // failure
} // end main