#if !defined (APART_MATH_H)

//define our vector 2 overloaded operations

struct v2
{
    union
    {
	struct
	{
	    r32 x, y;
	};
	r32 e[2];
    };

    inline v2 &operator*=(r32 a);

    inline v2 &operator+=(v2 a);

    inline v2 &operator-=(v2 a);
};

inline v2
V2(r32 x, r32 y)
{
    v2 result;
    result.x = x;
    result.y = y;

    return(result);
}

inline v2
operator*(r32 a, v2 b)
{
    v2 result;
    result.x = a * b.x;
    result.y = a * b.y;    

    return(result); 
}

inline v2
operator*(v2 b, r32 a)
{
    v2 result = a * b;

    return(result); 
}

inline v2
operator/(v2 b, r32 a)
{
    v2 result;
    result.x = b.x / a;
    result.y = b.y / a;
    return(result);
}

inline v2 &v2::
operator*=(r32 a)
{
    *this = a * *this;
    return(*this);
}

inline v2
operator-(v2 a)
{
    v2 result;
    result.x = -a.x;
    result.y = -a.y;    

    return(result);
}

inline v2
operator+(v2 a, v2 b)
{
    v2 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;

    return(result);
}

inline v2 &v2::
operator+=(v2 a)
{
    *this = *this + a;

    return(*this);
}

inline v2
operator-(v2 a, v2 b)
{
    v2 result;

    result.x = a.x - b.x;
    result.y = a.y - b.y;

    return(result);
}

inline v2 &v2::
operator-=(v2 a)
{
    *this = *this - a;

    return(*this);
}

inline r32
Square(r32 a)
{
    r32 result = a * a;

    return(result);
}

inline r32
Inner(v2 a, v2 b)
{
    r32 result = a.x * b.x + a.y * b.y;
    return(result);
}

internal r32
LengthSq(v2 a)
{
    r32 result = Inner(a, a);

    return(result);
}

internal r32
Length(v2 a)
{
    r32 result = (r32)sqrt(LengthSq(a));

    return(result);
}

internal v2
NormalizeV2(v2 v)
{
    v2 result = {};
    r32 m = SquareRoot((r32)(pow(v.x, 2.0) + pow(v.y, 2.0)));
    result.x = v.x / m;
    result.y = v.y / m;
    return(result);
}

internal v2
ReflectionVector(v2 v, v2 n)
{
    v2 result = v - 2 * Inner(v, n) * n;
    return(result);
}

internal bool32
InRange(v2 v, r32 min, r32 max)
{
    if ((v.x > min) && (v.x < max))
    {
	if ((v.y > min) && (v.y < max))
	{
	    return(true);
	}
    }
    return(false);
}
#define APART_MATH_H
#endif
