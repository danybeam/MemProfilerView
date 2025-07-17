module;
// Include headers that don't play nice with import
#pragma warning( push, 0 )
#include <Clay/clay.h>
#pragma warning( pop )

export module Maths:Vector;

import <algorithm>;

export namespace mem_profile_viewer
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
            return Vector2(this->x * rhs, this->y * rhs);
        }

        Vector2 operator -(const Vector2& rhs) const
        {
            return Vector2(this->x - rhs.x, this->y - rhs.y);
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
        /**
         * Clamp a vector so that its components are not lower than min nor bigger than max. This returns a new vector
         * @param min minimum value for the vector components
         * @param max maximum value for the vector components
         * @param dimension which dimension to clamp the vector on
         */
        Vector2 Clamped(float min, float max = .0f, CLAMP_DIMENSION dimension = CLAMP_DIMENSION::BOTH_DIMENSIONS) const;
    };
}

void mem_profile_viewer::Vector2::Clamp(float min, float max, CLAMP_DIMENSION dimension)
{
    if (dimension == CLAMP_DIMENSION::BOTH_DIMENSIONS || dimension == CLAMP_DIMENSION::X_DIMENSION)
    {
        this->x = std::clamp(this->x, min, max);
    }

    if (dimension == CLAMP_DIMENSION::BOTH_DIMENSIONS || dimension == CLAMP_DIMENSION::Y_DIMENSION)
    {
        this->y = std::clamp(this->y, min, max);
    }
}

mem_profile_viewer::Vector2 mem_profile_viewer::Vector2::Clamped(float min, float max, CLAMP_DIMENSION dimension) const
{
    Vector2 result = *this;

    result.Clamp(min, max, dimension);

    return result;
}
