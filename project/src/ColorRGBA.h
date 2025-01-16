#pragma once
#include "MathHelpers.h"
#include <algorithm>
#undef max

namespace dae
{
	struct ColorRGBA
	{
		float r{};
		float g{};
		float b{};
		float a{1};

		void MaxToOne()
		{
			const float maxValue = std::max(r, (std::max(g, b)));
			a = std::clamp(a,0.f, 1.f);
			if (maxValue > 1.f)
				*this /= maxValue;
		}

		static ColorRGBA Lerp(const ColorRGBA& c1, const ColorRGBA& c2, float factor)
		{
			return { Lerpf(c1.r, c2.r, factor), Lerpf(c1.g, c2.g, factor),
				Lerpf(c1.b, c2.b, factor),Lerpf(c1.a, c2.a, factor) };
		}

#pragma region ColorRGBA (Member) Operators
		const ColorRGBA& operator+=(const ColorRGBA& c)
		{
			r += c.r;
			g += c.g;
			b += c.b;

			return *this;
		}

		ColorRGBA operator+(const ColorRGBA& c) const
		{
			return { r + c.r, g + c.g, b + c.b, a  };
		}

		const ColorRGBA& operator-=(const ColorRGBA& c)
		{
			r -= c.r;
			g -= c.g;
			b -= c.b;

			return *this;
		}

		ColorRGBA operator-(const ColorRGBA& c) const
		{
			return { r - c.r, g - c.g, b - c.b, a };
		}

		const ColorRGBA& operator*=(const ColorRGBA& c)
		{
			r *= c.r;
			g *= c.g;
			b *= c.b;

			return *this;
		}

		ColorRGBA operator*(const ColorRGBA& c) const
		{
			return { r * c.r, g * c.g, b * c.b, a };
		}

		const ColorRGBA& operator/=(const ColorRGBA& c)
		{
			r /= c.r;
			g /= c.g;
			b /= c.b;

			return *this;
		}

		const ColorRGBA& operator*=(float s)
		{
			r *= s;
			g *= s;
			b *= s;

			return *this;
		}

		ColorRGBA operator*(float s) const
		{
			return { r * s, g * s,b * s, a };
		}

		const ColorRGBA& operator/=(float s)
		{
			r /= s;
			g /= s;
			b /= s;

			return *this;
		}

		ColorRGBA operator/(float s) const
		{
			return { r / s, g / s,b / s , a};
		}

#pragma endregion
	};


}