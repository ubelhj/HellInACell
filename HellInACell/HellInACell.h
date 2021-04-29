#pragma once

#include "bakkesmod/plugin/bakkesmodplugin.h"

constexpr auto plugin_version = "3.0";

#define nl(x) SettingsFile << std::string(x) << '\n'
#define blank SettingsFile << '\n'
#define cv(x) std::string(x)

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
    void write(int stat, int team);

    void updateTime();
    void writeTime(bool isOT, int seconds);

    void renderAllStrings();

    ServerWrapper getGameServer();

    void GenerateSettingsFile();
};

