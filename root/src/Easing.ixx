module;
#define _USE_MATH_DEFINES
#include <math.h>;

export module Maths:Easing;

import <functional>;

export namespace mem_profile_viewer
{
    using EasingFunction = std::function<float(float)>;

    //Linear ease
    struct Linear
    {
        static float Interpolation(float value) { return value; };
    };


    //Quadratic ease (in, out, inOut)
    struct Quadratic
    {
        static float In(float value)
        {
            return value * value;
        }

        static float Out(float value)
        {
            return value * (2.0f - value);
        }

        static float InOut(float value)
        {
            if (value < 0.5)
            {
                return 2 * value * value;
            }
            else
            {
                return (-2 * value * value) + (4 * value) - 1;
            }
        }
    };


    //Cubic ease (in, out, inOut)
    struct Cubic
    {
        static float In(float value)
        {
            return value * value * value;
        }

        static float Out(float value)
        {
            float f = (value - 1.0f);
            return f * f * f + 1.0f;
        }

        static float InOut(float value)
        {
            if (value < 0.5f)
            {
                return 4.0f * value * value * value;
            }
            else
            {
                float f = ((2.0f * value) - 2.0f);
                return 0.5f * f * f * f + 1.0f;
            }
        }
    };


    //Quartic ease (in, out, inOut)
    struct Quartic
    {
        static float In(float value)
        {
            return value * value * value * value;
        }

        static float Out(float value)
        {
            float f = (value - 1.0f);
            return f * f * f * (1.0f - value) + 1.0f;
        }

        static float InOut(float value)
        {
            if (value < 0.5f)
            {
                return 8.0f * value * value * value * value;
            }
            else
            {
                float f = (value - 1.0f);
                return -8.0f * f * f * f * f + 1.0f;
            }
        }
    };


    //Quintic ease (in, out, inOut)
    struct Quintic
    {
        static float In(float value)
        {
            return value * value * value * value * value;
        }

        static float Out(float value)
        {
            float f = (value - 1.0f);
            return f * f * f * f * f + 1.0f;
        }

        static float InOut(float value)
        {
            if (value < 0.5f)
            {
                return 16.0f * value * value * value * value * value;
            }
            else
            {
                float f = ((2.0f * value) - 2.0f);
                return 0.5f * f * f * f * f * f + 1.0f;
            }
        }
    };


    //Sinusoidal ease (in, out, inOut)
    struct Sinusoidal
    {
        static float In(float value)
        {
            return sinf((value - 1) * static_cast<float>(M_PI_2)) + 1;
        }

        static float Out(float value)
        {
            return sinf(value * static_cast<float>(M_PI_2));
        }

        static float InOut(float value)
        {
            return 0.5f * (1.0f - cosf(value * static_cast<float>(M_PI)));
        }
    };


    //Exponential ease (in, out, inOut)
    struct Exponential
    {
        static float In(float value)
        {
            return (value == 0.0f) ? value : powf(2, 10 * (value - 1));
        }

        static float Out(float value)
        {
            return (value == 1.0f) ? value : 1.0f - powf(2, -10 * value);
        }

        static float InOut(float value)
        {
            if (value == 0.0f || value == 1.0f) return value;

            if (value < 0.5f)
            {
                return 0.5f * powf(2, (20 * value) - 10);
            }
            else
            {
                return -0.5f * powf(2, (-20 * value) + 10) + 1;
            }
        }
    };


    //Circular ease (in, out, inOut)
    struct Circular
    {
        static float In(float value)
        {
            return 1.0f - sqrtf(1 - (value * value));
        }

        static float Out(float value)
        {
            return sqrtf((2.0f - value) * value);
        }

        static float InOut(float value)
        {
            if (value < 0.5f)
            {
                return 0.5f * (1.0f - sqrtf(1.0f - 4.0f * (value * value)));
            }
            else
            {
                return 0.5f * (sqrtf(-((2.0f * value) - 3.0f) * ((2.0f * value) - 1.0f)) + 1.0f);
            }
        }
    };


    //Elastic ease (in, out, inOut)
    struct Elastic
    {
        static float In(float value)
        {
            return sinf(13.0f * (float)M_PI_2 * value) * powf(2, 10 * (value - 1.0f));
        }

        static float Out(float value)
        {
            return sinf(-13.0f * (float)M_PI_2 * (value + 1.0f)) * powf(2, -10 * value) + 1.0f;
        }

        static float InOut(float value)
        {
            if (value < 0.5f)
            {
                return 0.5f * sinf(13.0f * (float)M_PI_2 * (2.0f * value)) * powf(2, 10 * ((2.0f * value) - 1.0f));
            }
            else
            {
                return 0.5f * (sinf(-13.0f * (float)M_PI_2 * ((2.0f * value - 1.0f) + 1.0f)) * powf(
                    2, -10 * (2.0f * value - 1.0f)) + 2.0f);
            }
        }
    };


    //Back ease (in, out, inOut)
    struct Back
    {
        static float In(float value)
        {
            return value * value * value - value * sinf(value * (float)M_PI);
        }

        static float Out(float value)
        {
            float f = (1.0f - value);
            return 1.0f - (f * f * f - f * sinf(f * (float)M_PI));
        }

        static float InOut(float value)
        {
            if (value < 0.5f)
            {
                float f = 2.0f * value;
                return 0.5f * (f * f * f - f * sinf(f * (float)M_PI));
            }
            else
            {
                float f = (1.0f - (2.0f * value - 1.0f));
                return 0.5f * (1.0f - (f * f * f - f * sinf(f * (float)M_PI))) + 0.5f;
            }
        }
    };


    //Bounce ease (in, out, inOut)
    struct Bounce
    {
        static float In(float value)
        {
            return 1.0f - Out(1.0f - value);
        }

        static float Out(float value)
        {
            if (value < 4.0f / 11.0f)
            {
                return (121.0f * value * value) / 16.0f;
            }
            else if (value < 8.0f / 11.0f)
            {
                return (363.0f / 40.0f * value * value) - (99.0f / 10.0f * value) + 17.0f / 5.0f;
            }
            else if (value < 9.0f / 10.0f)
            {
                return (4356.0f / 361.0f * value * value) - (35442.0f / 1805.0f * value) + 16061.0f / 1805.0f;
            }
            else
            {
                return (54.0f / 5.0f * value * value) - (513.0f / 25.0f * value) + 268.0f / 25.0f;
            }
        }

        static float InOut(float value)
        {
            if (value < 0.5f)
            {
                return 0.5f * In(value * 2.0f);
            }
            else
            {
                return 0.5f * Out(value * 2.0f - 1.0f) + 0.5f;
            }
        }
    };
}
