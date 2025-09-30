#pragma once

struct ButtonState {
	int halfTransitionsCount;
	bool endedDown;
};

#define ButtonHeldDown(btn) (!btn.endedDown)
#define ButtonClicked(btn) (btn.halfTransitionsCount)

struct Input {
	ButtonState uiClick;
	
	ButtonState startGame;
	ButtonState switchExitMenu;
	
	ButtonState scrollUp;
	ButtonState scrollDown;

	ButtonState forward;
	ButtonState backwards;
	ButtonState left;
	ButtonState right;
	ButtonState up;
	ButtonState down;

	ButtonState attack;
	ButtonState placeBlock;

	ButtonState openInventory;
	ButtonState inventorySlots[9]; // TODO: убрать захардкоженое число

	// DEBUG
	ButtonState rebuildShaders;
	ButtonState showDebugInfo;
};

void ProcessButtonInput(ButtonState* oldButtonState, ButtonState* newButtonState, bool isReleased);