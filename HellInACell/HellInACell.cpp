#include "pch.h"
#include "HellInACell.h"
#include "bakkesmod/wrappers/GameObject/Stats/StatEventWrapper.h"


BAKKESMOD_PLUGIN(HellInACell, "Hell in a cell scoreboard", plugin_version, PLUGINTYPE_THREADED)

// holds the stats recorded by each team (demos, exterms, goals, points)
int stats[8];
// start of team 2's stats
constexpr int team2 = 4;
// location of each stat within team sub-array
constexpr int demos = 0;
constexpr int exterms = 1;
constexpr int goals = 2;
constexpr int points = 3;
// size of stats array
constexpr int numStats = 8;
// point values of each stat (demos, exterms, goals)
int values[3];

// x and y location of overlay
float xLocation;
float yLocation;
// colors of overlay
int overlayColors[3];

// whether plugin is enabled
bool enabled = false;

// whether the user is a spectator
bool spectator = false;
// used to prevent spectators from having doubled stats
bool spectatorHasSeen = false;

void HellInACell::onLoad()
{
    // enables or disables plugin
    auto enableVar = cvarManager->registerCvar("hic_enable", "0", "enables hell in a cell scoreboard");
    if (enableVar.getBoolValue()) {
        enabled = true;
        hookEvents();
    };
    enableVar.addOnValueChanged([this](std::string, CVarWrapper cvar) {
        enabled = cvar.getBoolValue();
        if (enabled) {
            hookEvents();
        } else {
            unhookEvents();
        }
        });

    // enables or disables spectator mode (for some reason spectate causes double scoring otherwise)
    auto spectatorVar = cvarManager->registerCvar("hic_spectator", "0", "Whether or not the viewer is a spectator");
    spectator = spectatorVar.getBoolValue();
    spectatorVar.addOnValueChanged([this](std::string, CVarWrapper cvar) { spectator = cvar.getBoolValue(); });

    auto demoPointsVar = cvarManager->registerCvar("hic_demolition_value", "1", "set demolition point value");
    values[demos] = demoPointsVar.getIntValue();
    demoPointsVar.addOnValueChanged([this](std::string, CVarWrapper cvar) {
        values[demos] = cvar.getIntValue();
        });

    auto extermPointsVar = cvarManager->registerCvar("hic_extermination_value", "3", "set extermination point value");
    values[exterms] = extermPointsVar.getIntValue();
    extermPointsVar.addOnValueChanged([this](std::string, CVarWrapper cvar) {
        values[exterms] = cvar.getIntValue();
        });

    auto goalPointsVar = cvarManager->registerCvar("hic_goal_value", "5", "set goal point value");
    values[goals] = goalPointsVar.getIntValue();
    goalPointsVar.addOnValueChanged([this](std::string, CVarWrapper cvar) {
        values[goals] = cvar.getIntValue();
        });

    // sets cvar to move counter's X location
    auto xLocVar = cvarManager->registerCvar("hic_x_location", "0.86", "set location of scoreboard X in % of screen", true, true, 0.0, true, 1.0);
    xLocation = xLocVar.getFloatValue();
    xLocVar.addOnValueChanged([this](std::string, CVarWrapper cvar) {
        xLocation = cvar.getFloatValue();
        });

    // sets cvar to move counter's Y location
    auto yLocVar = cvarManager->registerCvar("hic_y_location", "0.025", "set location of scoreboard Y in % of screen", true, true, 0.0, true, 1.0);
    yLocation = yLocVar.getFloatValue();
    yLocVar.addOnValueChanged([this](std::string, CVarWrapper cvar) {
        yLocation = cvar.getFloatValue();
        });

    // overlay red value changer 
    auto overlayRedVar = cvarManager->registerCvar("hic_red", "255", "Red value in overlay", true, true, 0, true, 255);
    overlayColors[0] = overlayRedVar.getIntValue();
    overlayRedVar.addOnValueChanged([this](std::string, CVarWrapper cvar) {
        overlayColors[0] = cvar.getIntValue();
        });

    // overlay green value changer 
    auto overlayGreenVar = cvarManager->registerCvar("hic_green", "0", "green value in overlay", true, true, 0, true, 255);
    overlayColors[1] = overlayGreenVar.getIntValue();
    overlayGreenVar.addOnValueChanged([this](std::string, CVarWrapper cvar) {
        overlayColors[1] = cvar.getIntValue();
        });

    // overlay blue value changer 
    auto overlayBlueVar = cvarManager->registerCvar("hic_blue", "0", "blue value in overlay", true, true, 0, true, 255);
    overlayColors[2] = overlayBlueVar.getIntValue();
    overlayBlueVar.addOnValueChanged([this](std::string, CVarWrapper cvar) {
        overlayColors[2] = cvar.getIntValue();
        });

    gameWrapper->RegisterDrawable(std::bind(&HellInACell::render, this, std::placeholders::_1));
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

    if (receiver.IsNull()) {
        cvarManager->log("null receiver");
    }

    // blue is 0, orange is 1
    auto receiverTeam = receiver.GetTeamNum();
    //cvarManager->log(std::to_string(receiverTeam));

    // where in the array the team of the receiver starts
    int teamStart = team2 * receiverTeam;

    if (label.ToString().compare("Demolition") == 0) {
        if (spectator) {
            if (spectatorHasSeen) {
                spectatorHasSeen = false;
                return;
            } else {
                spectatorHasSeen = true;
            }
        }
        // if blue (0), stats[0]++
        // if orange (1), stats[4]++
        stats[teamStart + demos]++;
        // adds 1 demo to points of team
        stats[teamStart + points] = values[demos] * stats[teamStart + demos];
        return;
    }

    if (label.ToString().compare("Extermination") == 0) {
        if (spectator) {
            if (spectatorHasSeen) {
                spectatorHasSeen = false;
                return;
            }
            else {
                spectatorHasSeen = true;
            }
        }
        // if blue (0), stats[1]++
        // if orange (1), stats[5]++
        stats[teamStart + exterms]++;
        // adds 1 exterm to points of team
        stats[teamStart + points] = values[exterms] * stats[teamStart + exterms];
        return;
    }

    if (label.ToString().compare("Goal") == 0) {
        if (spectator) {
            if (spectatorHasSeen) {
                spectatorHasSeen = false;
                return;
            }
            else {
                spectatorHasSeen = true;
            }
        }
        // if blue (0), stats[2]++
        // if orange (1), stats[6]++
        stats[teamStart + goals]++;
        // adds 1 goal to points of team
        stats[teamStart + points] = values[goals] * stats[teamStart + goals];
        return;
    }
}

void HellInACell::startGame() {
    for (int i = 0; i < 6; i++) {
        stats[i] = 0;
    }
}

void HellInACell::render(CanvasWrapper canvas) {
    if (!enabled) {
        return;
    }

    Vector2 screen = canvas.GetSize();

    // in 1080p font size is 2
    // y value of 2 size text is approx 20
    float fontSize = ((float)screen.X / (float)1920) * 2;

    // sets to color from cvars
    canvas.SetColor(overlayColors[0], overlayColors[1], overlayColors[2], 255);

    
    canvas.SetPosition(Vector2({ int((float)screen.X * xLocation), int((float)screen.Y * yLocation) }));
    canvas.DrawString("Blue Team", fontSize, fontSize);
    canvas.SetPosition(Vector2({ int((float)screen.X * xLocation), int((fontSize * 11) + ((float)screen.Y * yLocation)) }));
    canvas.DrawString("Demolitions: " + std::to_string(stats[demos]), fontSize, fontSize);
    canvas.SetPosition(Vector2({ int((float)screen.X * xLocation), int((fontSize * 22) + ((float)screen.Y * yLocation)) }));
    canvas.DrawString("Exterminations: " + std::to_string(stats[exterms]), fontSize, fontSize);
    canvas.SetPosition(Vector2({ int((float)screen.X * xLocation), int((fontSize * 33) + ((float)screen.Y * yLocation)) }));
    canvas.DrawString("Points: " + std::to_string(stats[points]), fontSize, fontSize);
    canvas.SetPosition(Vector2({ int((float)screen.X * xLocation), int((fontSize * 44) + ((float)screen.Y * yLocation)) }));
    canvas.DrawString("Orange Team", fontSize, fontSize);
    canvas.SetPosition(Vector2({ int((float)screen.X * xLocation), int((fontSize * 55) + ((float)screen.Y * yLocation)) }));
    canvas.DrawString("Demolitions: " + std::to_string(stats[team2 + demos]), fontSize, fontSize);
    canvas.SetPosition(Vector2({ int((float)screen.X * xLocation), int((fontSize * 66) + ((float)screen.Y * yLocation)) }));
    canvas.DrawString("Exterminations: " + std::to_string(stats[team2 + exterms]), fontSize, fontSize);
    canvas.SetPosition(Vector2({ int((float)screen.X * xLocation), int((fontSize * 77) + ((float)screen.Y * yLocation)) }));
    canvas.DrawString("Points: " + std::to_string(stats[team2 + points]), fontSize, fontSize);
}

void HellInACell::onUnload()
{
}