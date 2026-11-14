#ifndef GARDEN_PRESENTATION_H
#define GARDEN_PRESENTATION_H

#include "game.h"
#include "audio_settings.h"
#include "bn_optional.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_palette_ptr.h"
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

enum class TitleChoice { play, almanac, count };

enum class PauseChoice { resume, restart, choose_levels, master, music, effects, count };
enum class Feedback { none, planted, removed, occupied, insufficient_sun, cooling_down, collected };

struct ViewState
{
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
};

class Presentation
{
public:
    Presentation();
    void update(const Game& game, const ViewState& view);

private:
    void change_screen(const Game& game);
    void draw_entities(const Game& game);
    void draw_tombstones(const Game& game, int pan = 0);
    void draw_hud(const Game& game, const ViewState& view);
    void draw_menu(const Game& game, const ViewState& view);
    void draw_title(const ViewState& view);
    void draw_preview(const Game& game);
    void draw_almanac(const Game& game, const ViewState& view);
    void put_entities_behind_overlay();
    void draw_loss(const Game& game);
    void draw_reward(const Game& game);
    void draw_level_select(const Game& game);
    void draw_wave_bar(const Game& game);
    void draw_feedback(const Game& game, const ViewState& view);
    void clear_entities();
    void flash_red(bn::sprite_ptr& sprite);
    static void actor(bn::optional<bn::sprite_ptr>& sprite, bool active,
                      int x, int y, int frame, int depth = 0);

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
    std::array<bn::optional<bn::sprite_ptr>, 3> _volume_sliders;
    std::array<bn::optional<bn::sprite_ptr>, 2> _title_plants;
    bn::optional<bn::sprite_ptr> _title_options;
    std::array<bn::optional<bn::sprite_ptr>, levels_per_world> _level_tiles;
    bn::optional<bn::sprite_ptr> _level_cursor;
    std::array<bn::optional<bn::sprite_ptr>, zombie_type_count> _gallery;
    bn::optional<bn::regular_bg_ptr> _almanac_panel;
    bn::vector<bn::sprite_ptr, 32> _entry_text;
    std::array<bn::optional<bn::sprite_ptr>, 3> _entry_icons;
    bn::optional<bn::sprite_ptr> _portrait;
    bn::optional<bn::sprite_ptr> _gallery_cursor;
    int _last_entry = -1;
    int _title_frame = 0;
    bn::vector<bn::sprite_ptr, 12> _preview_text;
    int _preview_pan = 0;
    int _loss_notice_stage = 0;
    int _seed_page = -1;
    int _bar_progress = -1;
    bn::optional<bn::sprite_ptr> _reward_packet;
    bn::optional<bn::sprite_ptr> _reward_shadow;
    std::array<bn::optional<bn::sprite_ptr>, rows * columns> _plants;
    std::array<bn::optional<bn::sprite_ptr>, zombie_capacity> _zombies;
    std::array<bn::optional<bn::sprite_ptr>, tombstone_capacity> _tombstones;
    std::array<bn::optional<bn::sprite_palette_ptr>, 3> _frost_palettes;
    bn::optional<bn::sprite_palette_ptr> _hit_palette;
    std::array<bn::optional<bn::sprite_ptr>, shot_capacity> _shots;
    std::array<bn::optional<bn::sprite_ptr>, sun_capacity> _suns;
    std::array<bn::optional<bn::sprite_ptr>, rows> _mowers;
    int _last_sun = -1;
    int _last_pause_choice = -1;
    int _last_audio = -1;
};
}
#endif
