module;
// Include headers that don't play nice with import
#pragma warning( push, 0 )
#include <Clay/clay.h>
#pragma warning( pop )

export module Maths:Vector;

import <algorithm>;

export namespace memProfileViewer
{
    /**
     * Enum to indicate which dimension(s) to clamp
     */
    enum class CLAMP_DIMENSION
    {
        BOTH_DIMENSIONS,
        X_DIMENSION,
        Y_DIMENSION,
    };
    
    /**
     * Class to hold a 2-dimensional vector
     */
    class Vector2
    {
    public:
        float x, y; /**< the X and Y components of the vector */
        Vector2() = default; /**< default constructor */

        /**
         * Constructor for implicit constructions
         * @param x x component of the vector
         * @param y y component of the vector
         */
        Vector2(float x, float y) : x(x), y(y)
        {
        }

        /**
         * Add two vectors together.
         * @param rhs vector to add to this
         * @return reference to this vector
         */
        Vector2& operator +=(const Vector2& rhs)
        {
            x += rhs.x;
            y += rhs.y;
            return *this;
        }

        /**
         * Multiply the components of a vector by an integer.
         * @param rhs number to multiply the components by
         * @return a new vector with both components multiplied by rhs
         */
        Vector2 operator *(const int& rhs) const
        {
            return Vector2(this->x*rhs, this->y*rhs);
        }

        /**
         * conversion operator to convert a memProfileViewer::Vector2 to a Clay_Vector2
         */
        operator Clay_Vector2() const { return {x, y}; }

    public:
        /**
         * Clamp a vector so that its components are not lower than min nor bigger than max
         * @param min minimum value for the vector components
         * @param max maximum value for the vector components
         * @param dimension which dimension to clamp the vector on
         */
        void Clamp(float min, float max = .0f, CLAMP_DIMENSION dimension = CLAMP_DIMENSION::BOTH_DIMENSIONS);
    };
}

void memProfileViewer::Vector2::Clamp(float min, float max, CLAMP_DIMENSION dimension)
{
    if (dimension == CLAMP_DIMENSION::BOTH_DIMENSIONS || dimension == CLAMP_DIMENSION::X_DIMENSION)
    {
        // TODO(danybeam) this was to have clamped min but unclamped max but it needs to be more explicit
        float minValue = min == 0 ? this->x - 100 : min;
        float maxValue = max == 0 ? this->x + 100 : max;
        this->x = std::clamp(this->x, minValue, maxValue);
    }

    if (dimension == CLAMP_DIMENSION::BOTH_DIMENSIONS || dimension == CLAMP_DIMENSION::Y_DIMENSION)
    {
        float minValue = min == 0 ? this->y - 100 : min;
        float maxValue = max == 0 ? this->y + 100 : max;
        this->y = std::clamp(this->y, minValue, maxValue);
    }
}
