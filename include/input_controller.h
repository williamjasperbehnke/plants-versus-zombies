#ifndef GARDEN_INPUT_CONTROLLER_H
#define GARDEN_INPUT_CONTROLLER_H
#include "audio_commands.h"
#include "input_frame.h"
#include "view_state.h"

namespace garden
{
// Owns screen interaction and transient view state, with no hardware or renderer.
class InputController
{
public:
    InputController(const InputController&) = delete;
    InputController& operator=(const InputController&) = delete;
    InputController(Game& game, SaveSlots& slots) : _game(game), _slots(slots) {}
    void update(const InputFrame& input);
    void finish_frame();
    bool holding_sky_sun() const { return _tutorial.holds_sky_sun(); }
    bool holding_waves() const { return _tutorial.active(); }
    const ViewState& view() const { return _view; }
    const AudioCommands& audio_commands() const { return _commands; }
private:
    void reset_view();
    void handle_save_input();
    void refresh_saves();
    void start_game();
    void show_title();
    void show_level_select();
    void handle_input();
    void handle_almanac_input();
    void handle_preview_input();
    void handle_seed_select_input();
    void handle_lost_input();
    void handle_level_select_input();
    void handle_title_input();
    void handle_won_input();
    void handle_paused_input();
    void handle_playing_input();
    void feedback(Feedback kind);
    TutorialGuide _tutorial;
    bool _collected = false;
    Game& _game;
    SaveSlots& _slots;
    ViewState _view;
    InputFrame _input;
    AudioCommands _commands;
    std::array<KeyRepeat, 4> _repeat{};
    std::array<KeyRepeat, 2> _seed_repeat{};
};
}
#endif
