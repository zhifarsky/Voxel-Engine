#pragma once
#include <GLFW/glfw3.h>

#define IsMouseButtonReleased(window, button) (glfwGetMouseButton(window, button) == GLFW_RELEASE)
#define IsKeyReleased(window, key) (glfwGetKey(window, key) == GLFW_RELEASE)

struct ButtonState {
	int halfTransitionsCount;
	bool endedDown;
};

#define ButtonHeldDown(btn) (!btn.endedDown)
#define ButtonClicked(btn) (btn.halfTransitionsCount)

struct Input {
	ButtonState startGame;
	ButtonState switchExitMenu;
	
	ButtonState scrollUp;
	ButtonState scrollDown;

	ButtonState forward;
	ButtonState backwards;
	ButtonState left;
	ButtonState right;

	ButtonState attack;
	ButtonState placeBlock;

	ButtonState inventorySlots[9]; // TODO: убрать захардкоженое число

	// DEBUG
	ButtonState rebuildShaders;
};

void ProcessButtonInput(ButtonState* oldButtonState, ButtonState* newButtonState, bool isReleased);