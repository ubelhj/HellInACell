#include "pch.h"
#include "HellInACell.h"
#include "bakkesmod/wrappers/GameObject/Stats/StatEventWrapper.h"


BAKKESMOD_PLUGIN(HellInACell, "Hell in a cell scoreboard", plugin_version, PLUGINTYPE_THREADED)

// holds the stats recorded by each team (demos, exterms, goals)
int stats[6];
// values of each stat (demos, exterms, goals)
int values[3];
// x and y location of overlay
float xLocation;
float yLocation;
// colors of overlay
int overlayColors[3];

void HellInACell::onLoad()
{
    // enables or disables plugin
    auto enableVar = cvarManager->registerCvar("hic_enable", "0", "enables hell in a cell scoreboard");
    if (enableVar.getBoolValue()) {
        hookEvents;
    };
    enableVar.addOnValueChanged([this](std::string, CVarWrapper cvar) {
        if (cvar.getBoolValue()) {
            hookEvents;
        } else {
            unhookEvents;
        }
        });

    hookEvents();
}

void HellInACell::hookEvents() {
    gameWrapper->HookEventWithCaller<ServerWrapper>("Function TAGame.GFxHUD_TA.HandleStatTickerMessage",
        std::bind(&HellInACell::statEvent, this, std::placeholders::_1, std::placeholders::_2));

    // works on a starting game 
    gameWrapper->HookEventPost("Function GameEvent_Soccar_TA.WaitingForPlayers.BeginState", std::bind(&HellInACell::startGame, this));
    // works on a joined game in progress
    gameWrapper->HookEventPost("Function Engine.PlayerInput.InitInputSystem", std::bind(&HellInACell::startGame, this));
}

void HellInACell::unhookEvents() {
    gameWrapper->UnhookEvent("Function TAGame.GFxHUD_TA.HandleStatTickerMessage");
    gameWrapper->UnhookEvent("Function GameEvent_Soccar_TA.WaitingForPlayers.BeginState");
    gameWrapper->UnhookEvent("Function Engine.PlayerInput.InitInputSystem");
}

struct TheArgStruct
{
    uintptr_t Receiver;
    uintptr_t Victim;
    uintptr_t StatEvent;
};

// called whenever a stat appears in the top right and decides whether to update a file
void HellInACell::statEvent(ServerWrapper caller, void* args) {
    auto tArgs = (TheArgStruct*)args;
    //cvarManager->log("stat event!");

    auto victim = PriWrapper(tArgs->Victim);
    auto receiver = PriWrapper(tArgs->Receiver);
    auto statEvent = StatEventWrapper(tArgs->StatEvent);
    auto label = statEvent.GetLabel();
    //cvarManager->log(label.ToString());
}

void HellInACell::startGame() {

}

void HellInACell::onUnload()
{
}