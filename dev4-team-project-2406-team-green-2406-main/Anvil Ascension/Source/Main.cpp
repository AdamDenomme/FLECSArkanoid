// handles everything
#include "Application.h"
// program entry point
int main()
{
	Application exampleSpaceGame;
	if (exampleSpaceGame.Init()) {
		if (exampleSpaceGame.Run()) {
			return exampleSpaceGame.Shutdown() ? 0 : 1;
		}
	}
	return 1;
}