module;
// Include like this for compatibility reasons
#include <flecs.h>;

export module IOState;

import Maths;

export namespace mem_profile_viewer
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
        static constexpr float mouse_wheel_acceleration_base = 5.0f;
        float mouse_wheel_acceleration = 5.0f;
        mem_profile_viewer::Vector2 prev_mouse_wheel = {0,0}; /**< How much the mouse wheel was scrolled last frame */
        mem_profile_viewer::Vector2 current_mouse_wheel = {0,0}; /**< How much the mouse wheel has scrolled */
        mem_profile_viewer::Vector2 target_mouse_wheel = {0,0}; /**< where we want the mouse wheel to be */

        Tween mouse_wheel_acceleration_tween = Tween(mouse_wheel_acceleration);
        Tween current_mouse_wheel_tween_x = Tween(current_mouse_wheel.x);
        Tween current_mouse_wheel_tween_y = Tween(current_mouse_wheel.y);
    };
}

module :private;

mem_profile_viewer::IOStateModule::IOStateModule(flecs::world& world)
{
    world.module<IOStateModule>();

    world.component<IOState_Component>("IOState_Component")
    .member<mem_profile_viewer::Vector2>("mouse_wheel");

    world.emplace<IOState_Component>();
}
