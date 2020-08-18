#include "pch.h"
#include "HellInACell.h"
#include "bakkesmod/wrappers/GameObject/Stats/StatEventWrapper.h"
#include <iostream>
#include <fstream>

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
float xLocationBlue;
float yLocationBlue;
float xLocationOrange;
float yLocationOrange;
// colors of overlay
// 0-2 RGB of Blue team
// 3-5 RGB of Orange team
int overlayColors[6];

// whether plugin is enabled
bool enabled = false;

// whether the user is a spectator
bool spectator = false;
// used to prevent spectators from having doubled stats
bool spectatorHasSeen = false;

std::string fileNames[8] = {
    "blueDemolitions",
    "blueExterminations",
    "blueGoals",
    "bluePoints",
    "orangeDemolitions",
    "orangeExterminations",
    "orangeGoals",
    "orangePoints"
};

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

    // sets cvar to move blue team counter's X location
    auto xLocBlueVar = cvarManager->registerCvar("hic_blue_x_location", "0.86", "set location of blue scoreboard X in % of screen", true, true, 0.0, true, 1.0);
    xLocationBlue = xLocBlueVar.getFloatValue();
    xLocBlueVar.addOnValueChanged([this](std::string, CVarWrapper cvar) {
        xLocationBlue = cvar.getFloatValue();
        });

    // sets cvar to move blue team counter's Y location
    auto yLocBlueVar = cvarManager->registerCvar("hic_blue_y_location", "0.0", "set location of blue scoreboard Y in % of screen", true, true, 0.0, true, 1.0);
    yLocationBlue = yLocBlueVar.getFloatValue();
    yLocBlueVar.addOnValueChanged([this](std::string, CVarWrapper cvar) {
        yLocationBlue = cvar.getFloatValue();
        });

    // sets cvar to move orange team counter's X location
    auto xLocOrangeVar = cvarManager->registerCvar("hic_orange_x_location", "0.86", "set location of orange scoreboard X in % of screen", true, true, 0.0, true, 1.0);
    xLocationOrange = xLocOrangeVar.getFloatValue();
    xLocOrangeVar.addOnValueChanged([this](std::string, CVarWrapper cvar) {
        xLocationOrange = cvar.getFloatValue();
        });

    // sets cvar to move orange team counter's Y location
    auto yLocOrangeVar = cvarManager->registerCvar("hic_orange_y_location", "0.08", "set location of orange scoreboard Y in % of screen", true, true, 0.0, true, 1.0);
    yLocationOrange = yLocOrangeVar.getFloatValue();
    yLocOrangeVar.addOnValueChanged([this](std::string, CVarWrapper cvar) {
        yLocationOrange = cvar.getFloatValue();
        });

    // overlay blue team red value changer 
    auto overlayBlueRedVar = cvarManager->registerCvar("hic_blue_red", "0", "blue team red value in overlay", true, true, 0, true, 255);
    overlayColors[0] = overlayBlueRedVar.getIntValue();
    overlayBlueRedVar.addOnValueChanged([this](std::string, CVarWrapper cvar) {
        overlayColors[0] = cvar.getIntValue();
        });

    // overlay blue team green value changer 
    auto overlayBlueGreenVar = cvarManager->registerCvar("hic_blue_green", "181", "blue team green value in overlay", true, true, 0, true, 255);
    overlayColors[1] = overlayBlueGreenVar.getIntValue();
    overlayBlueGreenVar.addOnValueChanged([this](std::string, CVarWrapper cvar) {
        overlayColors[1] = cvar.getIntValue();
        });

    // overlay blue team blue value changer 
    auto overlayBlueBlueVar = cvarManager->registerCvar("hic_blue_blue", "255", "blue team blue value in overlay", true, true, 0, true, 255);
    overlayColors[2] = overlayBlueBlueVar.getIntValue();
    overlayBlueBlueVar.addOnValueChanged([this](std::string, CVarWrapper cvar) {
        overlayColors[2] = cvar.getIntValue();
        });

    // overlay orange team red value changer 
    auto overlayOrangeRedVar = cvarManager->registerCvar("hic_orange_red", "255", "orange team red value in overlay", true, true, 0, true, 255);
    overlayColors[3] = overlayOrangeRedVar.getIntValue();
    overlayOrangeRedVar.addOnValueChanged([this](std::string, CVarWrapper cvar) {
        overlayColors[3] = cvar.getIntValue();
        });

    // overlay orange team green value changer 
    auto overlayOrangeGreenVar = cvarManager->registerCvar("hic_orange_green", "98", "orange team green value in overlay", true, true, 0, true, 255);
    overlayColors[4] = overlayOrangeGreenVar.getIntValue();
    overlayOrangeGreenVar.addOnValueChanged([this](std::string, CVarWrapper cvar) {
        overlayColors[4] = cvar.getIntValue();
        });

    // overlay orange team blue value changer 
    auto overlayOrangeBlueVar = cvarManager->registerCvar("hic_orange_blue", "0", "orange team blue value in overlay", true, true, 0, true, 255);
    overlayColors[5] = overlayOrangeBlueVar.getIntValue();
    overlayOrangeBlueVar.addOnValueChanged([this](std::string, CVarWrapper cvar) {
        overlayColors[5] = cvar.getIntValue();
        });

    cvarManager->registerNotifier("hic_reset",
        [this](std::vector<std::string> params) {
            startGame();
        }, "Resets all stats", PERMISSION_ALL);

    gameWrapper->RegisterDrawable(std::bind(&HellInACell::render, this, std::placeholders::_1));

    startGame();
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
        stats[teamStart + points] += values[demos];
        write(demos, receiverTeam);
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
        stats[teamStart + points] += values[exterms];
        write(exterms, receiverTeam);
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
        stats[teamStart + points] += values[goals];
        write(goals, receiverTeam);
        return;
    }
}

void HellInACell::startGame() {
    for (int i = 0; i < numStats; i++) {
        stats[i] = 0;
    }

    for (int i = 0; i < 3; i++) {
        write(i, 0);
        write(i, 1);
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

    // blue team strings
    // sets to color from cvars
    canvas.SetColor(overlayColors[0], overlayColors[1], overlayColors[2], 255);
    canvas.SetPosition(Vector2({ int((float)screen.X * xLocationBlue), int((float)screen.Y * yLocationBlue) }));
    canvas.DrawString("Blue Team", fontSize, fontSize);
    canvas.SetPosition(Vector2({ int((float)screen.X * xLocationBlue), int((fontSize * 11) + ((float)screen.Y * yLocationBlue)) }));
    canvas.DrawString("Demolitions: " + std::to_string(stats[demos]), fontSize, fontSize);
    canvas.SetPosition(Vector2({ int((float)screen.X * xLocationBlue), int((fontSize * 22) + ((float)screen.Y * yLocationBlue)) }));
    canvas.DrawString("Exterminations: " + std::to_string(stats[exterms]), fontSize, fontSize);
    canvas.SetPosition(Vector2({ int((float)screen.X * xLocationBlue), int((fontSize * 33) + ((float)screen.Y * yLocationBlue)) }));
    canvas.DrawString("Points: " + std::to_string(stats[points]), fontSize, fontSize);
    // orange team strings
    canvas.SetColor(overlayColors[3], overlayColors[4], overlayColors[5], 255);
    canvas.SetPosition(Vector2({ int((float)screen.X * xLocationOrange), int((float)screen.Y * yLocationOrange) }));
    canvas.DrawString("Orange Team", fontSize, fontSize);
    canvas.SetPosition(Vector2({ int((float)screen.X * xLocationOrange), int((fontSize * 11) + ((float)screen.Y * yLocationOrange)) }));
    canvas.DrawString("Demolitions: " + std::to_string(stats[team2 + demos]), fontSize, fontSize);
    canvas.SetPosition(Vector2({ int((float)screen.X * xLocationOrange), int((fontSize * 22) + ((float)screen.Y * yLocationOrange)) }));
    canvas.DrawString("Exterminations: " + std::to_string(stats[team2 + exterms]), fontSize, fontSize);
    canvas.SetPosition(Vector2({ int((float)screen.X * xLocationOrange), int((fontSize * 33) + ((float)screen.Y * yLocationOrange)) }));
    canvas.DrawString("Points: " + std::to_string(stats[team2 + points]), fontSize, fontSize);
}

void HellInACell::write(int stat, int team) {
    // where in the array the team of the receiver starts
    int teamStart = team2 * team;

    // writes the stat file
    std::ofstream statFile;
    statFile.open("./HellInACell/" + fileNames[stat + teamStart] + ".txt");
    statFile << stats[stat + teamStart];
    statFile.close();

    // writes the points file
    std::ofstream pointsFile;
    pointsFile.open("./HellInACell/" + fileNames[points + teamStart] + ".txt");
    pointsFile << stats[points + teamStart];
    pointsFile.close();
}

void HellInACell::onUnload()
{
}