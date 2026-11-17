#ifndef GARDEN_VIEW_STATE_H
#define GARDEN_VIEW_STATE_H
#include "tutorial.h"
#include "audio_settings.h"

namespace garden
{
struct Cursor
{
    int row = 2;
    int column = 0;
    PlantType seed = PlantType::sunflower;
};

enum class TitleChoice { play, almanac, count };

enum class PauseChoice { resume, restart, choose_levels, master, music, effects, count };
enum class Feedback { none, planted, removed, occupied, insufficient_sun, cooling_down, collected };

struct ViewState
{
    TutorialPrompt tutorial;
    AudioSettings audio;
    Cursor cursor;
    Feedback feedback = Feedback::none;
    int feedback_frames = 0;
    int feedback_x = 0;
    int feedback_y = 0;
    PlantType feedback_seed = PlantType::sunflower;
    TitleChoice title_choice = TitleChoice::play;
    PauseChoice pause_choice = PauseChoice::resume;
    bool shoveling = false;
    bool almanac_zombies = false;
    bool almanac_details = false;
    int almanac_entry = 0;
    int almanac_scroll = 0;
    int seed_choice = 0;
    int seed_error_frames = 0;
};

}
#endif
