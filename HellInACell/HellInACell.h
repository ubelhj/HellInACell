#pragma once

#include "bakkesmod/plugin/bakkesmodplugin.h"

constexpr auto plugin_version = "1.0";

class HellInACell: public BakkesMod::Plugin::BakkesModPlugin
{

	//Boilerplate
	virtual void onLoad();
	virtual void onUnload();

	void hookEvents();
	void unhookEvents();

	void startGame();
	void statEvent(ServerWrapper caller, void* args);
	void render(CanvasWrapper canvas);
};

