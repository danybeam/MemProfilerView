module;
// Include like this for compatibility reasons
#include <flecs.h>;

export module IOState;

import Maths;

export namespace memProfileViewer
{
    /**
     * Class to define the flecs module
     */
    class IOStateModule
    {
    public:
        /**
         * Constructor to register the module using flecs import
         * @param world reference to the world to register the module on
         */
        IOStateModule(flecs::world& world);
    };

    /**
     * Structure to store the current and target state of the IO devices.
     *
     * @remark This is an evergreen structure. It should grow and shrink as needed.
     */
    struct IOState_Component
    {
        // TODO(danybeam) add target mouse wheel state for tweening
        memProfileViewer::Vector2 mouse_wheel = {0,0}; /**< How much the mouse whell has scrolled */
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
