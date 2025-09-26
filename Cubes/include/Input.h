#pragma once
#include <GLFW/glfw3.h>

#define IsMouseButtonReleased(window, button) (glfwGetMouseButton(window, button) == GLFW_RELEASE)
#define IsKeyReleased(window, key) (glfwGetKey(window, key) == GLFW_RELEASE)

struct ButtonState {
	int halfTransitionsCount;
	bool endedDown;
};

#define ButtonHeldDown(btn) !btn.endedDown
#define ButtonClicked(btn) btn.halfTransitionsCount && btn.endedDown

struct Input {
	ButtonState forward;
	ButtonState backwards;
	ButtonState left;
	ButtonState right;

	ButtonState attack;
	ButtonState placeBlock;

	ButtonState inventoryNextItem;
	ButtonState inventoryPrevItem;

	ButtonState startGame;
	ButtonState switchExitMenu;
};

void ProcessButtonInput(ButtonState* oldButtonState, ButtonState* newButtonState, bool isReleased);