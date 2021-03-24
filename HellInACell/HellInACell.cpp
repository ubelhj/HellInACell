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
std::string renderStrings[numStats];

std::string renderPrefixes[] = {
    "Blue Team",
    "Demolitions: ", 
    "Exterminations: ",
    "Points: ",
    "Orange Team"
};

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
LinearColor blueColorBG;
LinearColor orangeColorBG;
float fontSize;

// where files are stored
std::filesystem::path fileLocation;
//std::string fileLocation = "./HellInACell/";

// whether plugin is enabled
bool enabled = false;
bool enabledBG = false;

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
    fileLocation = gameWrapper->GetDataFolder() / "HellInACell";

    // enables or disables plugin
    auto enableVar = cvarManager->registerCvar("hic_enable", "0", "enables hell in a cell scoreboard");
    enableVar.addOnValueChanged([this](std::string, CVarWrapper cvar) {
        enabled = cvar.getBoolValue();
        if (enabled) {
            hookEvents();
        } else {
            unhookEvents();
        }
        });

    // enables or disables background
    auto enableBGVar = cvarManager->registerCvar("hic_bg_enable", "0", "enables hell in a cell background");
    enableBGVar.addOnValueChanged([this](std::string, CVarWrapper cvar) {
        enabledBG = cvar.getBoolValue();
        });

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

    auto fontSizeVar = cvarManager->registerCvar("hic_font_size", "2.0", "set font size");
    fontSize = fontSizeVar.getFloatValue();
    fontSizeVar.addOnValueChanged([this](std::string, CVarWrapper cvar) {
        fontSize = cvar.getFloatValue();
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
    auto blueColorVar = cvarManager->registerCvar("hic_blue_color", "#00b5ff", "color of blue overlay");
    blueColor = blueColorVar.getColorValue();
    blueColorVar.addOnValueChanged([this](std::string, CVarWrapper cvar) {
        blueColor = cvar.getColorValue();
        });

    // color of blue team scoreboard background
    auto blueBGColorVar = cvarManager->registerCvar("hic_blue_color_bg", "#0000008C", "color of blue overlay bakcground");
    blueColorBG = blueBGColorVar.getColorValue();
    blueBGColorVar.addOnValueChanged([this](std::string, CVarWrapper cvar) {
        blueColorBG = cvar.getColorValue();
        });

    // color of orange team scoreboard
    auto orangeColorVar = cvarManager->registerCvar("hic_orange_color", "#ff6200", "color of orange overlay");
    orangeColor = orangeColorVar.getColorValue();
    orangeColorVar.addOnValueChanged([this](std::string, CVarWrapper cvar) {
        orangeColor = cvar.getColorValue();
        });

    // color of orange team scoreboard background
    auto orangeBGColorVar = cvarManager->registerCvar("hic_orange_color_bg", "#0000008C", "color of orange overlay bakcground");
    orangeColorBG = orangeBGColorVar.getColorValue();
    orangeBGColorVar.addOnValueChanged([this](std::string, CVarWrapper cvar) {
        orangeColorBG = cvar.getColorValue();
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

    cvarManager->registerCvar("hic_set_render_blue_team", renderPrefixes[0], "set blue team render string, default Blue Team")
        .addOnValueChanged([this](std::string, auto cvar) {
        renderPrefixes[0] = cvar.getStringValue();
        renderAllStrings();
            });

    cvarManager->registerCvar("hic_set_render_demolitions", renderPrefixes[1], "set demolition render string, default Demolitons: ")
        .addOnValueChanged([this](std::string, auto cvar) {
        renderPrefixes[1] = cvar.getStringValue();
        renderAllStrings();
            });

    cvarManager->registerCvar("hic_set_render_exterminations", renderPrefixes[2], "set extermination render string, default Exterminations: ")
        .addOnValueChanged([this](std::string, auto cvar) {
        renderPrefixes[2] = cvar.getStringValue();
        renderAllStrings();
            });

    cvarManager->registerCvar("hic_set_render_points", renderPrefixes[3], "set points render string, default Points: ")
        .addOnValueChanged([this](std::string, auto cvar) {
        renderPrefixes[3] = cvar.getStringValue();
        renderAllStrings();
            });

    cvarManager->registerCvar("hic_set_render_orange_team", renderPrefixes[4], "set orange team render string")
        .addOnValueChanged([this](std::string, auto cvar) {
        renderPrefixes[4] = cvar.getStringValue();
        renderAllStrings();
            });

    gameWrapper->RegisterDrawable(std::bind(&HellInACell::render, this, std::placeholders::_1));

    namespace fs = std::filesystem;
    fs::create_directory(fileLocation);

    startGame();
}

void HellInACell::hookEvents() {
    gameWrapper->HookEventWithCallerPost<ServerWrapper>("Function TAGame.GFxHUD_TA.HandleStatTickerMessage",
        std::bind(&HellInACell::statEvent, this, std::placeholders::_1, std::placeholders::_2));

    // works on a starting game 
    gameWrapper->HookEventPost("Function GameEvent_Soccar_TA.WaitingForPlayers.BeginState", std::bind(&HellInACell::startGame, this));
    // works on a joined game in progress
    gameWrapper->HookEventPost("Function Engine.PlayerInput.InitInputSystem", std::bind(&HellInACell::startGame, this));
    // hooks when the scoreboard time is updated
    gameWrapper->HookEventPost("Function TAGame.GameEvent_Soccar_TA.OnGameTimeUpdated",
        std::bind(&HellInACell::updateTime, this));
}

void HellInACell::unhookEvents() {
    gameWrapper->UnhookEventPost("Function TAGame.GFxHUD_TA.HandleStatTickerMessage");
    gameWrapper->UnhookEventPost("Function GameEvent_Soccar_TA.WaitingForPlayers.BeginState");
    gameWrapper->UnhookEventPost("Function Engine.PlayerInput.InitInputSystem");
    gameWrapper->UnhookEventPost("Function TAGame.GameEvent_Soccar_TA.OnGameTimeUpdated");
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
    PlayerControllerWrapper controller = gameWrapper->GetPlayerController();

    if (controller) {
        PriWrapper pri = controller.GetPRI();

        if (pri) {
            unsigned int teamNum = pri.GetTeamNum2();

            if (teamNum != 0 && teamNum != 1) {
                // player is spectator
                if (spectatorHasSeen) {
                    spectatorHasSeen = false;
                    return;
                }
                else {
                    spectatorHasSeen = true;
                }
            }
        }    
    }

    /*if (spectator) {
        if (spectatorHasSeen) {
            spectatorHasSeen = false;
            return;
        }
        else {
            spectatorHasSeen = true;
        }
    }*/

    // if blue (0), stats[0]++
    // if orange (1), stats[4]++
    stats[teamStart + eventType]++;
    // adds 1 demo to points of team
    stats[teamStart + points] += values[eventType];
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

    // writes time as basic 5 minute game by default
    writeTime(0, 300);
    renderAllStrings();
}

void HellInACell::render(CanvasWrapper canvas) {
    if (!enabled) {
        return;
    }

    Vector2 screen = canvas.GetSize();

    Vector2 blueLoc({ int((float)screen.X * xLocationBlue), int((float)screen.Y * yLocationBlue) });
    Vector2 orangeLoc({ int((float)screen.X * xLocationOrange), int((float)screen.Y * yLocationOrange) });

    // in 1080p font size is 2
    // y value of 2 size text is approx 20
    //float fontSize = ((float)screen.X / (float)1920) * 2;

    Vector2F maxStringSizeBlue = { -1.0, -1.0 };
    for (int i = 0; i < 4; i++) {
        Vector2F newStringSize = canvas.GetStringSize(renderStrings[i], fontSize, fontSize);

        if (newStringSize.X > maxStringSizeBlue.X) {
            maxStringSizeBlue.X = newStringSize.X;
        }

        if (newStringSize.Y > maxStringSizeBlue.Y) {
            maxStringSizeBlue.Y = newStringSize.Y;
        }
    }

    Vector2F maxStringSizeOrange = { -1.0, -1.0 };
    for (int i = 4; i < 8; i++) {
        Vector2F newStringSize = canvas.GetStringSize(renderStrings[i], fontSize, fontSize);

        if (newStringSize.X > maxStringSizeOrange.X) {
            maxStringSizeOrange.X = newStringSize.X;
        }

        if (newStringSize.Y > maxStringSizeOrange.Y) {
            maxStringSizeOrange.Y = newStringSize.Y;
        }
    }

    if (enabledBG) {
        canvas.SetColor(blueColorBG);
        canvas.SetPosition(blueLoc);
        canvas.FillBox(Vector2({ int(maxStringSizeBlue.X), int(maxStringSizeBlue.Y * 4) }));

        canvas.SetColor(orangeColorBG);
        canvas.SetPosition(orangeLoc);
        canvas.FillBox(Vector2({ int(maxStringSizeOrange.X), int(maxStringSizeOrange.Y * 4) }));
    }

    // blue team strings
    // sets to color from cvars
    canvas.SetColor(blueColor);
    for (int i = 0; i < 4; i++) {
        // locates based on screen and font size
        canvas.SetPosition(blueLoc);
        blueLoc.Y += maxStringSizeBlue.Y;
        // does averages if the user wants them and if a stat has an average
        //std::string renderString = statToRenderString(overlayStats[i], overlayAverages[i]);
        //int width = renderString.length() * fontSize * 10;
        canvas.DrawString(renderStrings[i], fontSize, fontSize, false);
    }
    // orange team strings
    canvas.SetColor(orangeColor);
    for (int i = 4; i < 8; i++) {
        // locates based on screen and font size
        canvas.SetPosition(orangeLoc);
        orangeLoc.Y += maxStringSizeOrange.Y;
        // does averages if the user wants them and if a stat has an average
        //std::string renderString = statToRenderString(overlayStats[i], overlayAverages[i]);
        //int width = renderString.length() * fontSize * 10;
        canvas.DrawString(renderStrings[i], fontSize, fontSize, false);
    }
    
}

void HellInACell::renderAllStrings() {
    renderStrings[0] = renderPrefixes[0];
    renderStrings[1] = renderPrefixes[1] + std::to_string(stats[demos]);
    renderStrings[2] = renderPrefixes[2] + std::to_string(stats[exterms]);
    renderStrings[3] = renderPrefixes[3] + std::to_string(stats[points]);
    // orange team strings
    renderStrings[4] = renderPrefixes[4];
    renderStrings[5] = renderPrefixes[1] + std::to_string(stats[team2 + demos]);
    renderStrings[6] = renderPrefixes[2] + std::to_string(stats[team2 + exterms]);
    renderStrings[7] = renderPrefixes[3] + std::to_string(stats[team2 + points]);
}

void HellInACell::write(int stat, int team) {
    // where in the array the team of the receiver starts
    int teamStart = team2 * team;

    // writes the stat file
    std::ofstream statFile (fileLocation / (fileNames[stat + teamStart] + ".txt"));
    statFile << stats[stat + teamStart];
    statFile.close();

    // writes the points file
    std::ofstream pointsFile (fileLocation / (fileNames[points + teamStart] + ".txt"));
    pointsFile << stats[points + teamStart];
    pointsFile.close();

    renderAllStrings();
}

void HellInACell::updateTime() {
    auto sw = getGameServer();

    if (sw.IsNull()) {
        cvarManager->log("unable to check time, not in game");
        return;
    }

    auto seconds = sw.GetSecondsRemaining();
    bool isOT = sw.GetbOverTime();

    writeTime(isOT, seconds);
}

void HellInACell::writeTime(bool isOT, int seconds) {
    int minutes = seconds / 60;
    int remSeconds = seconds % 60;

    // writes the time file
    std::ofstream statFile (fileLocation / "time.txt");
    statFile << std::to_string(minutes) + ":";
    // if remSeconds is 1 character long, makes sure to make it 2 characters
    if (remSeconds / 10 == 0) {
        statFile << "0" + std::to_string(remSeconds);
    } else {
        statFile << std::to_string(remSeconds);
    }
    statFile.close();

    renderAllStrings();
}

ServerWrapper HellInACell::getGameServer() {
    if (gameWrapper->IsInOnlineGame()) {
        return gameWrapper->GetOnlineGame();
    } 
    return gameWrapper->GetGameEventAsServer();
}

void HellInACell::onUnload()
{
}