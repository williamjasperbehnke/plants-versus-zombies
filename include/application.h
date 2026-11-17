#ifndef GARDEN_APPLICATION_H
#define GARDEN_APPLICATION_H
#include "audio_player.h"
#include "input_controller.h"
#include "presentation.h"
#include "save_store.h"

namespace garden
{
// Composition root: only frame ordering and persistence belong here.
class Application
{
public:
    Application() = default;
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    void run();
private:
    SaveStore _save;
    Game _game;
    InputController _controller{_game};
    AudioPlayer _audio;
    Presentation _presentation;
};
}
#endif
