#ifndef GARDEN_VIEW_STATE_H
#define GARDEN_VIEW_STATE_H
#include "audio/audio_types.h"
#include "game/tutorial.h"
#include "save/progress.h"

namespace garden {
struct Cursor {
    int row = 2;
    int column = 0;
    PlantType seed = PlantType::sunflower;
};

enum class TitleChoice {
    play,
    almanac,
    count
};

enum class PauseChoice {
    resume,
    restart,
    choose_levels,
    master,
    music,
    effects,
    count
};
enum class Feedback {
    none,
    planted,
    removed,
    occupied,
    insufficient_sun,
    cooling_down,
    collected
};

struct AlmanacState {
    bool zombies = false;
    bool details = false;
    int entry = 0;
    int scroll = 0;

    bool operator==(const AlmanacState& other) const {
        return zombies == other.zombies && details == other.details && entry == other.entry &&
               scroll == other.scroll;
    }
};

struct SaveSlotSummary {
    bool occupied = false;
    int unlocked = 1;
    int cleared = 0;

    bool operator==(const SaveSlotSummary& other) const {
        return occupied == other.occupied && unlocked == other.unlocked && cleared == other.cleared;
    }
};

struct SaveMenuState {
    std::array<SaveSlotSummary, save_slot_count> slots{};
    int slot = 0;
    bool delete_selected = false;
    bool confirming_delete = false;
    bool confirm_delete = false;

    bool operator==(const SaveMenuState& other) const {
        return slots == other.slots && slot == other.slot && delete_selected == other.delete_selected &&
               confirming_delete == other.confirming_delete && confirm_delete == other.confirm_delete;
    }
};

struct ViewState {
    SaveMenuState save_menu;
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
    AlmanacState almanac;
    int seed_choice = 0;
    int seed_error_frames = 0;
};

} // namespace garden
#endif
