#ifndef GARDEN_APPLICATION_H
#define GARDEN_APPLICATION_H
#include "app/input_controller.h"
#include "audio/audio_player.h"
#include "save/save_store.h"
#include "ui/presentation.h"

namespace garden {
// Composition root: only frame ordering and persistence belong here.
class Application {
public:
    Application() = default;
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    void run();

private:
    SaveStore _save;
#ifdef GARDEN_DEBUG_SAVE
    SaveSlots _slots{true};
#else
    SaveSlots _slots;
#endif
    Game _game;
    InputController _controller{_game, _slots};
    AudioPlayer _audio;
    Presentation _presentation;
};
} // namespace garden
#endif
