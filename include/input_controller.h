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
    explicit InputController(Game& game) : _game(game) {}
    void update(const InputFrame& input);
    void finish_frame();
    bool holding_sky_sun() const { return _tutorial.holds_sky_sun(); }
    bool holding_waves() const { return _tutorial.active(); }
    const ViewState& view() const { return _view; }
    const AudioCommands& audio_commands() const { return _commands; }
private:
    void reset_view();
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
    ViewState _view;
    InputFrame _input;
    AudioCommands _commands;
    std::array<KeyRepeat, 4> _repeat{};
    std::array<KeyRepeat, 2> _seed_repeat{};
};
}
#endif
