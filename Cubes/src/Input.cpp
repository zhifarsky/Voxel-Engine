#include "input.h"

void ProcessButtonInput(ButtonState* oldButtonState, ButtonState* newButtonState, bool isReleased) {
	newButtonState->endedDown = isReleased;
	newButtonState->halfTransitionsCount = oldButtonState->endedDown == true && newButtonState->endedDown == false;
	//newButtonState->halfTransitionsCount = oldButtonState->endedDown != newButtonState->endedDown ? 1 : 0;
}