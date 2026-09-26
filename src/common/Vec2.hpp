#pragma once

class Vec2 {
public:
    float x;
    float y;

    Vec2();
    Vec2(float xValue, float yValue);

    float Length() const;
    Vec2 Normalized() const;
    Vec2 operator+(const Vec2& other) const;
    Vec2 operator-(const Vec2& other) const;
    Vec2 operator*(float scalar) const;
    Vec2& operator+=(const Vec2& other);
};
