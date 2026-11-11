#ifndef GARDEN_PRESENTATION_H
#define GARDEN_PRESENTATION_H

#include "game.h"
#include "bn_optional.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_string.h"
#include "bn_vector.h"

namespace garden
{
struct Cursor
{
    int row = 2;
    int column = 0;
    PlantType seed = PlantType::sunflower;
};

enum class PauseChoice { resume, restart, title, count };
enum class Feedback { none, planted, removed, occupied, insufficient_sun, cooling_down, collected };

struct ViewState
{
    Cursor cursor;
    Feedback feedback = Feedback::none;
    int feedback_frames = 0;
    int feedback_x = 0;
    int feedback_y = 0;
    PlantType feedback_seed = PlantType::sunflower;
    PauseChoice pause_choice = PauseChoice::resume;
    bool shoveling = false;
};

class Presentation
{
public:
    Presentation();
    void update(const Game& game, const ViewState& view);

private:
    void change_screen(Phase phase);
    void draw_entities(const Game& game);
    void draw_hud(const Game& game, const ViewState& view);
    void draw_menu(const Game& game, const ViewState& view);
    void draw_wave_bar(const Game& game);
    void draw_feedback(const Game& game, const ViewState& view);
    void clear_entities();
    static void actor(bn::optional<bn::sprite_ptr>& sprite, bool active,
                      int x, int y, int frame, int depth = 0);

    Phase _screen = Phase::title;
    bn::regular_bg_ptr _background;
    bn::optional<bn::regular_bg_ptr> _menu;
    bn::sprite_text_generator _text;
    bn::sprite_ptr _cursor;
    bn::sprite_ptr _selection;
    bn::vector<bn::sprite_ptr, 20> _hud;
    std::array<bn::optional<bn::sprite_ptr>, plant_count> _seed_bars;
    std::array<bn::optional<bn::sprite_ptr>, wave_count> _wave_bar;
    bn::optional<bn::sprite_ptr> _wave_head;
    bn::optional<bn::sprite_ptr> _phase_icon;
    bn::optional<bn::sprite_ptr> _feedback_icon;
    std::array<bn::optional<bn::sprite_ptr>, rows * columns> _plants;
    std::array<bn::optional<bn::sprite_ptr>, zombie_capacity> _zombies;
    std::array<bn::optional<bn::sprite_ptr>, shot_capacity> _shots;
    std::array<bn::optional<bn::sprite_ptr>, sun_capacity> _suns;
    std::array<bn::optional<bn::sprite_ptr>, rows> _mowers;
    int _last_sun = -1;
    int _last_pause_choice = -1;
};
}
#endif
