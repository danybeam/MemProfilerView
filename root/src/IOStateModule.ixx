module;
#include <flecs.h>;

export module IOState;

import Maths;

export namespace memProfileViewer
{
    class IOStateModule
    {
    public:
        IOStateModule(flecs::world& world);
    };

    struct IOState_Component
    {
        memProfileViewer::Vector2 mouse_wheel = {0,0};
    };
}

module :private;

memProfileViewer::IOStateModule::IOStateModule(flecs::world& world)
{
    world.module<IOStateModule>();

    world.component<IOState_Component>("IOState_Component")
    .member<memProfileViewer::Vector2>("mouse_wheel");

    world.emplace<IOState_Component>();
}
