#include "app/application.h"
#include "bn_core.h"

int main() {
    bn::core::init();
    garden::Application application;
    application.run();
}
