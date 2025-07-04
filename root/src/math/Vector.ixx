module;
#pragma warning( push, 0 )
#include <algorithm>
#include <Clay/clay.h>
#pragma warning( pop )

export module Maths:Vector;

export namespace memProfileViewer
{
    enum class CLAMP_DIMENSION
    {
        BOTH_DIMENSIONS,
        X_DIMENSION,
        Y_DIMENSION,
    };

    class Vector2
    {
    public:
        float x, y;
        Vector2() = default;

        Vector2(float x, float y) : x(x), y(y)
        {
        }

        Vector2& operator +=(const Vector2& rhs)
        {
            x += rhs.x;
            y += rhs.y;
            return *this;
        }

        Vector2 operator *(const int& rhs)
        {
            return Vector2(this->x*rhs, this->y*rhs);
        }

        operator Clay_Vector2() const { return {x, y}; }

    public:
        void Clamp(float min, float max = .0f, CLAMP_DIMENSION dimension = CLAMP_DIMENSION::BOTH_DIMENSIONS);
    };
}

void memProfileViewer::Vector2::Clamp(float min, float max, CLAMP_DIMENSION dimension)
{
    if (dimension == CLAMP_DIMENSION::BOTH_DIMENSIONS || dimension == CLAMP_DIMENSION::X_DIMENSION)
    {
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
