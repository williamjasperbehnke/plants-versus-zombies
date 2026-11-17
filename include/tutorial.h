#ifndef GARDEN_TUTORIAL_H
#define GARDEN_TUTORIAL_H
#include "game.h"
namespace garden
{
inline constexpr int tutorial_sun_delay = 90;
enum class TutorialStep { none, plant_pea, collect_sun, shovel, plant_sunflower, sunflower_sun };
struct TutorialPrompt
{
    TutorialStep step = TutorialStep::none;
    int row = 2;
    int column = 0;
};
// Owns ordered first-play lessons and the actions allowed during each step.
class TutorialGuide
{
public:
    void begin(const Game& game)
    {
        _prompt = {};
        if(game.level_completed(game.level())) { return; }
        if(game.level() == 0) { _prompt.step = TutorialStep::plant_pea; }
        if(game.level() == 1)
        { _prompt = {TutorialStep::shovel, tutorial_weeds[0].row, tutorial_weeds[0].column}; }
    }
    void update(const Game& game, bool collected)
    {
        if(game.phase() != Phase::playing) { return; }
        bool pea = false, flower = false, wilted = false;
        for(int i = 0; i < rows * columns; ++i)
        {
            const auto& plant = game.plants()[i];
            if(! plant.active) { continue; }
            if(plant.wilted)
            { wilted = true; }
            else
            { pea |= plant.type == PlantType::peashooter; flower |= plant.type == PlantType::sunflower; }
        }
        switch(_prompt.step)
        {
        case TutorialStep::plant_pea: if(pea) { _prompt.step = TutorialStep::collect_sun; } break;
        case TutorialStep::collect_sun: if(collected) { _prompt.step = TutorialStep::none; } break;
        case TutorialStep::shovel:
            if(! wilted) { show_flower_lesson(); }
            else
            {
                for(const auto& cell : tutorial_weeds)
                {
                    const auto& plant = game.plants()[cell_index(cell.row, cell.column)];
                    if(plant.active && plant.wilted)
                    { _prompt.row = cell.row; _prompt.column = cell.column; break; }
                }
            }
            break;
        case TutorialStep::plant_sunflower:
            if(flower) { _prompt.step = TutorialStep::sunflower_sun; }
            break;
        case TutorialStep::sunflower_sun:
            if(! flower) { show_flower_lesson(); break; }
            if(collected)
            { _prompt.step = TutorialStep::none; }
            break;
        default: break;
        }
    }
    bool holds_sky_sun() const { return active() && _prompt.step != TutorialStep::collect_sun; }
    bool active() const { return _prompt.step != TutorialStep::none; }
    bool allows_plant(int row, int column, PlantType type) const
    {
        if(! active()) { return true; }
        if(row != _prompt.row || column != _prompt.column) { return false; }
        return (_prompt.step == TutorialStep::plant_pea && type == PlantType::peashooter) ||
               (_prompt.step == TutorialStep::plant_sunflower && type == PlantType::sunflower);
    }
    bool allows_shovel(int row, int column) const
    { return ! active() || (_prompt.step == TutorialStep::shovel && row == _prompt.row && column == _prompt.column); }
    bool allows_collect() const
    { return ! active() || _prompt.step == TutorialStep::collect_sun || _prompt.step == TutorialStep::sunflower_sun; }
    TutorialPrompt prompt() const { return _prompt; }
private:
    void show_flower_lesson()
    { _prompt = {TutorialStep::plant_sunflower, 2, 0}; }
    TutorialPrompt _prompt;
};
}
#endif
