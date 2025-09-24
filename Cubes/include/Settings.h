#pragma once
#include "stdio.h"

#define SettingsPath "settings"

struct Settings {
	float FOV;
	int renderDistance;
	int antiAliasingQuality;
	int shadowQuality;
};

void SettingsCreateDefault(Settings* settings);
void SettingsSave(Settings* settings);
void SettingsLoad(Settings* settings);
