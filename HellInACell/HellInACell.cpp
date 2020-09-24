#include "pch.h"
#include "HellInACell.h"
#include "bakkesmod/wrappers/GameObject/Stats/StatEventWrapper.h"
#include <iostream>
#include <fstream>
#include <filesystem>

BAKKESMOD_PLUGIN(HellInACell, "Hell in a cell scoreboard", plugin_version, PLUGINTYPE_THREADED)

// size of stats array
constexpr int numStats = 8;

// holds the stats recorded by each team (demos, exterms, goals, points)
int stats[numStats];

// location of each stat within team sub-array
enum stats {
    demos,
    exterms,
    goals, 
    points, 
    // start of team 2's stats
    team2
};

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
LinearColor blueColor;
LinearColor orangeColor;

// where files are stored
std::string fileLocation = "./HellInACell/";

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

const std::map<std::string, int> eventDictionary = {
    { "Demolition", demos},
    { "Extermination", exterms},
    { "Goal", goals}
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

    // color of blue team scoreboard
    auto blueColorVar = cvarManager->registerCvar("hic_blue_color", "#00b5ff", "color of overlay");
    blueColor = blueColorVar.getColorValue();
    blueColorVar.addOnValueChanged([this](std::string, CVarWrapper cvar) {
        blueColor = cvar.getColorValue();
        });

    // color of orange team scoreboard
    auto orangeColorVar = cvarManager->registerCvar("hic_orange_color", "#ff6200", "color of overlay");
    orangeColor = orangeColorVar.getColorValue();
    orangeColorVar.addOnValueChanged([this](std::string, CVarWrapper cvar) {
        orangeColor = cvar.getColorValue();
        });

    cvarManager->registerNotifier("hic_reset",
        [this](std::vector<std::string> params) {
            startGame();
        }, "Resets all stats", PERMISSION_ALL);

    cvarManager->registerCvar("hic_orange_set_demos", "0", "set orange team demos", true, true, 0, false, 0, false)
        .addOnValueChanged([this](std::string, auto cvar) {
            // makes sure points are properly set
            // ex if demos go from 1 -> 2, points should add 1
            // if demos go from 2 -> 1, points should subtract 1
            int difference = cvar.getIntValue() - stats[demos + team2];
            stats[demos + team2] = cvar.getIntValue();
            stats[points + team2] += difference * values[demos];
            write(demos, 1);
            });
    cvarManager->registerCvar("hic_orange_set_exterms", "0", "set orange team exterms", true, true, 0, false, 0, false)
        .addOnValueChanged([this](std::string, auto cvar) {
            int difference = cvar.getIntValue() - stats[exterms + team2];
            stats[exterms + team2] = cvar.getIntValue();
            stats[points + team2] += difference * values[exterms];
            write(exterms, 1);
            });
    cvarManager->registerCvar("hic_orange_set_goals", "0", "set orange team goals", true, true, 0, false, 0, false)
        .addOnValueChanged([this](std::string, auto cvar) {
            int difference = cvar.getIntValue() - stats[goals + team2];
            stats[goals + team2] = cvar.getIntValue();
            stats[points + team2] += difference * values[goals];
            write(goals, 1);
            });
    cvarManager->registerCvar("hic_orange_set_points", "0", "set orange team points", true, true, 0, false, 0, false)
        .addOnValueChanged([this](std::string, auto cvar) {
            stats[points + team2] = cvar.getIntValue();
            write(demos, 1);
            });
    cvarManager->registerCvar("hic_blue_set_demos", "0", "set blue team demos", true, true, 0, false, 0, false)
        .addOnValueChanged([this](std::string, auto cvar) { 
            int difference = cvar.getIntValue() - stats[demos];
            stats[demos] = cvar.getIntValue();
            stats[points] += difference * values[demos];
            write(demos, 0);
            });
    cvarManager->registerCvar("hic_blue_set_exterms", "0", "set blue team exterms", true, true, 0, false, 0, false)
        .addOnValueChanged([this](std::string, auto cvar) {
            int difference = cvar.getIntValue() - stats[exterms];
            stats[exterms] = cvar.getIntValue();
            stats[points] += difference * values[exterms];
            write(exterms, 0);
            });
    cvarManager->registerCvar("hic_blue_set_goals", "0", "set blue team goals", true, true, 0, false, 0, false)
        .addOnValueChanged([this](std::string, auto cvar) {
            int difference = cvar.getIntValue() - stats[goals];
            stats[goals] = cvar.getIntValue();
            stats[points] += difference * values[goals];
            write(goals, 0);
            });
    cvarManager->registerCvar("hic_blue_set_points", "0", "set blue team points", true, true, 0, false, 0, false)
        .addOnValueChanged([this](std::string, auto cvar) {
            stats[points] = cvar.getIntValue();
            write(goals, 0);
            });


    gameWrapper->RegisterDrawable(std::bind(&HellInACell::render, this, std::placeholders::_1));

    namespace fs = std::filesystem;
    fs::create_directory(fileLocation);

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

    auto eventTypePtr = eventDictionary.find(label.ToString());

    int eventType;

    if (eventTypePtr != eventDictionary.end()) {
        eventType = eventTypePtr->second;
        //cvarManager->log("event type: " + label.ToString());
        //cvarManager->log("event num: " + std::to_string(eventType));
    }
    else {
        //cvarManager->log("unused stat: " + label.ToString());
        return;
    }

    // checks for spectator mode
    // for some reason spectators get doubled stat events, so this accounts for that
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
    stats[teamStart + eventType]++;
    // adds 1 demo to points of team
    stats[teamStart + points] += values[eventType];
    //updateScore();
    write(eventType, receiverTeam);
    return;
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
    canvas.SetColor(blueColor);
    Vector2 blueLoc = Vector2({ int((float)screen.X * xLocationBlue), int((float)screen.Y * yLocationBlue) });
    canvas.SetPosition(blueLoc);
    canvas.DrawString("Blue Team", fontSize, fontSize);
    blueLoc += Vector2({ 0, int(fontSize * 11) });
    canvas.SetPosition(blueLoc);
    canvas.DrawString("Demolitions: " + std::to_string(stats[demos]), fontSize, fontSize);
    blueLoc += Vector2({ 0, int(fontSize * 11) });
    canvas.SetPosition(blueLoc);
    canvas.DrawString("Exterminations: " + std::to_string(stats[exterms]), fontSize, fontSize);
    blueLoc += Vector2({ 0, int(fontSize * 11) });
    canvas.SetPosition(blueLoc);
    canvas.DrawString("Points: " + std::to_string(stats[points]), fontSize, fontSize);
    // orange team strings
    canvas.SetColor(orangeColor);
    Vector2 orangeLoc = Vector2({ int((float)screen.X * xLocationOrange), int((float)screen.Y * yLocationOrange) });
    canvas.SetPosition(orangeLoc);
    canvas.DrawString("Orange Team", fontSize, fontSize);
    orangeLoc += Vector2({ 0, int(fontSize * 11) });
    canvas.SetPosition(orangeLoc);
    canvas.DrawString("Demolitions: " + std::to_string(stats[team2 + demos]), fontSize, fontSize);
    orangeLoc += Vector2({ 0, int(fontSize * 11) });
    canvas.SetPosition(orangeLoc);
    canvas.DrawString("Exterminations: " + std::to_string(stats[team2 + exterms]), fontSize, fontSize);
    orangeLoc += Vector2({ 0, int(fontSize * 11) });
    canvas.SetPosition(orangeLoc);
    canvas.DrawString("Points: " + std::to_string(stats[team2 + points]), fontSize, fontSize);
}

void HellInACell::write(int stat, int team) {
    // where in the array the team of the receiver starts
    int teamStart = team2 * team;

    // writes the stat file
    std::ofstream statFile;
    statFile.open(fileLocation + fileNames[stat + teamStart] + ".txt");
    statFile << stats[stat + teamStart];
    statFile.close();

    // writes the points file
    std::ofstream pointsFile;
    pointsFile.open(fileLocation + fileNames[points + teamStart] + ".txt");
    pointsFile << stats[points + teamStart];
    pointsFile.close();
}

void HellInACell::onUnload()
{
}