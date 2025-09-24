#include "Settings.h"
#include "Files.h"

void SettingsCreateDefault(Settings* settings)
{
	settings->FOV = 90;
	settings->renderDistance = 10;
	settings->antiAliasingQuality = 4;
	settings->shadowQuality = 12;
}

void SettingsSave(Settings* settings) {
	FILE* fileStream = fopen(SettingsPath, "wb");
	if (!fileStream)
		return;

	SerializeVariable(fileStream, settings->FOV);
	SerializeVariable(fileStream, settings->renderDistance);
	SerializeVariable(fileStream, settings->antiAliasingQuality);
	SerializeVariable(fileStream, settings->shadowQuality);
	fclose(fileStream);
}

void SettingsLoad(Settings* settings) {
	FILE* fileStream = fopen(SettingsPath, "rb");
	if (!fileStream){
		SettingsCreateDefault(settings);
		SettingsSave(settings);
		return;
	}

	DeserializeVariable(fileStream, settings->FOV);
	DeserializeVariable(fileStream, settings->renderDistance);
	DeserializeVariable(fileStream, settings->antiAliasingQuality);
	DeserializeVariable(fileStream, settings->shadowQuality);
	fclose(fileStream);
}