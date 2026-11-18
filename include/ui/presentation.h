#ifndef GARDEN_PRESENTATION_H
#define GARDEN_PRESENTATION_H

#include "bn_optional.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"
#include "game/game.h"
#include "ui/almanac_view.h"
#include "ui/menu_views.h"
#include "ui/save_menu_view.h"
#include "ui/view_state.h"
#include "ui/world_renderer.h"

namespace garden {
class Presentation {
public:
    Presentation();
    void update(const Game& game, const ViewState& view);

private:
    void change_screen(const Game& game);
    void draw_tutorial(const Game& game, const ViewState& view);
    void draw_hud(const Game& game, const ViewState& view);
    void draw_menu(const Game& game, const ViewState& view);
    void draw_title(const ViewState& view);
    void draw_preview(const Game& game);
    void draw_road(const Game& game, int pan, bool labels);
    void put_entities_behind_overlay();
    void draw_loss(const Game& game);
    void draw_reward(const Game& game);
    void draw_level_select(const Game& game);
    void draw_wave_bar(const Game& game);
    void draw_feedback(const Game& game, const ViewState& view);
    void clear_entities();

    bn::optional<bn::regular_bg_ptr> _tutorial_panel;
    bn::optional<bn::sprite_ptr> _lesson_target;
    bn::vector<bn::sprite_ptr, 12> _lesson_text;
    TutorialStep _lesson_step = TutorialStep::none;
    Phase _screen = Phase::title;
    bn::regular_bg_ptr _background;
    bn::optional<bn::regular_bg_ptr> _menu;
    bn::sprite_text_generator _text;
    bn::sprite_text_generator _sun_text;
    bn::sprite_ptr _cursor;
    bn::sprite_ptr _selection;
    bn::vector<bn::sprite_ptr, 48> _hud;
    std::array<bn::optional<bn::sprite_ptr>, plant_count> _seed_packets;
    std::array<bn::optional<bn::sprite_ptr>, wave_count> _wave_bar;
    bn::optional<bn::sprite_ptr> _wave_head;
    bn::optional<bn::sprite_ptr> _phase_icon;
    std::array<bn::optional<bn::sprite_ptr>, 2> _seed_arrows;
    bn::optional<bn::sprite_ptr> _feedback_icon;
    std::array<bn::optional<bn::sprite_ptr>, 2> _title_plants;
    bn::optional<bn::sprite_ptr> _title_options;
    std::array<bn::optional<bn::sprite_ptr>, levels_per_world> _level_tiles;
    bn::optional<bn::sprite_ptr> _level_cursor;
    std::array<bn::optional<bn::sprite_ptr>, zombie_type_count> _road_zombies;
    int _menu_frame = 0;
    bn::vector<bn::sprite_ptr, 16> _preview_text;
    std::array<bn::optional<bn::sprite_ptr>, 2> _preview_heading;
    int _preview_pan = 0;
    int _loss_notice_stage = 0;
    int _seed_page = -1;
    int _bar_progress = -1;
    bn::optional<bn::sprite_ptr> _reward_packet;
    bn::optional<bn::sprite_ptr> _reward_shadow;
    WorldRenderer _world;
    AlmanacView _almanac;
    SaveMenuView _save_menu;
    PauseMenuView _pause_menu;
    SeedSelectionView _seed_selection;
    int _last_level = -1;
    bool _reward_caption_drawn = false;
    int _last_sun = -1;
};
} // namespace garden
#endif
