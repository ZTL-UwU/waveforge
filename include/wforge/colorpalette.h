#ifndef WFORGE_COLORPALETTE_H
#define WFORGE_COLORPALETTE_H

#include <SFML/Graphics/Color.hpp>
#include <array>
#include <cstdint>

namespace wf {

struct ColorPaletteEntry {
	const char *name;
	sf::Color color;        // in RGBA
	sf::Color active_color; // electricity blend
};

constexpr sf::Color ui_text_color(std::uint8_t a) {
	return sf::Color(0, 0, 0, a);
}

// All indexed colors must be here, for dynamic generated textures
// Colors in static assets (e.g. PNG files) can be outside this palette
constexpr ColorPaletteEntry _colors[] = {
	{
		.name = "Air",
		.color = sf::Color(0, 0, 0, 0),
	},
	{
		.name = "Stone1",
		.color = sf::Color(96, 96, 96, 255),
	},
	{
		.name = "Stone2",
		.color = sf::Color(128, 128, 128, 255),
	},
	{
		.name = "Stone3",
		.color = sf::Color(144, 144, 144, 255),
	},
	{
		.name = "Stone4",
		.color = sf::Color(182, 182, 182, 255),
	},
	{
		.name = "Wood1",
		.color = sf::Color(228, 202, 167, 255),
	},
	{
		.name = "Wood2",
		.color = sf::Color(209, 177, 135, 255),
	},
	{
		.name = "Wood3",
		.color = sf::Color(186, 145, 88, 255),
	},
	{
		.name = "Copper1",
		.color = sf::Color(194, 107, 76, 255),
		.active_color = sf::Color(2, 177, 240, 255),
	},
	{
		.name = "Copper2",
		.color = sf::Color(201, 129, 104, 255),
		.active_color = sf::Color(93, 196, 233, 255),
	},
	{
		.name = "Copper3", // holder
		.color = sf::Color(87, 55, 8, 255),
		.active_color = sf::Color(87, 55, 8, 255),
	},
	{
		.name = "Copper4", // holder
		.color = sf::Color(97, 63, 13, 255),
		.active_color = sf::Color(97, 63, 13, 255),
	},
	{
		.name = "Copper5", // legacy copper color
		.color = sf::Color(184, 115, 51, 255),
		.active_color = sf::Color(2, 177, 240, 255),
	},
	{
		.name = "Sand1",
		.color = sf::Color(218, 207, 163, 255),
	},
	{
		.name = "Sand2", // darker
		.color = sf::Color(198, 174, 113, 255),
	},
	{
		.name = "Water",
		.color = sf::Color(64, 164, 223, 200),
	},
	{
		.name = "Oil",
		.color = sf::Color(85, 107, 47, 200),
	},
	{
		.name = "Smoke1", // light gray
		.color = sf::Color(200, 200, 200, 180),
	},
	{
		.name = "Smoke2", // dark gray
		.color = sf::Color(100, 100, 100, 180),
	},
	{
		.name = "Steam1", // very light blue
		.color = sf::Color(220, 240, 255, 150),
	},
	{
		.name = "Steam2", // light blue
		.color = sf::Color(180, 220, 255, 150),
	},
	{
		.name = "Fire1", // orange
		.color = sf::Color(255, 69, 0, 255),
	},
	{
		.name = "Fire2", // yellow
		.color = sf::Color(255, 215, 0, 255),
	},
	{
		.name = "Fire3", // red orange
		.color = sf::Color(255, 140, 0, 255),
	},
	{
		.name = "Electric",
		.color = sf::Color(0, 242, 255, 255),
	},
	{
		.name = "Laser",
		.color = sf::Color(51, 255, 184, 200),
	},
	{
		.name = "LaserStroke",
		.color = sf::Color(146, 226, 80, 255),
	},
	{
		.name = "POIMarker",
		.color = sf::Color(255, 0, 0, 40),
	},
	{
		.name = "Ruin",
		.color = sf::Color(128, 128, 128, 255),
	},
	{
		.name = "DebugRed",
		.color = sf::Color(255, 0, 0, 255),
	}
};

constexpr int _color_palette_size = sizeof(_colors) / sizeof(ColorPaletteEntry);
static_assert(_color_palette_size <= 255, "Too many colors in palette");

// Returns color index for name, or -1 if not found
// HINT: define color index variables as small ints (like 8/16 bits)
// so compile fails if -1 is returned
inline consteval unsigned int colorIndexOf(const char *name) {
	for (unsigned int i = 0; i < _color_palette_size; ++i) {
		auto j = _colors[i].name;
		auto k = name;
		// Cannot use std::strcmp in consteval context
		while (*j != '\0' && *k != '\0' && *j == *k) {
			++j;
			++k;
		}
		if (*j == '\0' && *k == '\0') {
			return i;
		}
	}
	return -1;
}

inline constexpr sf::Color colorOfIndex(unsigned int index) {
	return _colors[index].color;
}

inline constexpr ColorPaletteEntry colorPaletteOfIndex(unsigned int index) {
	return _colors[index];
}

inline consteval sf::Color colorOfName(const char *name) {
	return colorOfIndex(colorIndexOf(name));
}

inline consteval unsigned int packColorByNameNoAlpha(const char *name) {
	auto color = colorOfName(name);
	color.a = 255;
	return color.toInteger();
}

inline consteval unsigned int packColorByName(const char *name) {
	return colorOfName(name).toInteger();
}

inline constexpr sf::Color blendColor(
	const sf::Color overlay, const sf::Color base
) {
	const uint32_t alpha_numerator = static_cast<uint32_t>(overlay.a) * 255
		+ static_cast<uint32_t>(base.a) * (255 - overlay.a);
	const uint8_t out_a = (alpha_numerator + 127) / 255;

	if (out_a == 0) {
		return {0, 0, 0, 0};
	}

	auto compute_channel =
		[out_a](
			uint8_t fg_val, uint8_t fg_a, uint8_t bg_val, uint8_t bg_a
		) constexpr -> uint8_t {
		const uint32_t numerator = static_cast<uint32_t>(fg_val) * fg_a * 255
			+ static_cast<uint32_t>(bg_val) * bg_a * (255 - fg_a);

		const uint32_t denominator = static_cast<uint32_t>(out_a) * 255;
		return static_cast<uint8_t>((numerator + out_a * 127) / denominator);
	};

	const uint8_t r = compute_channel(overlay.r, overlay.a, base.r, base.a);
	const uint8_t g = compute_channel(overlay.g, overlay.a, base.g, base.a);
	const uint8_t b = compute_channel(overlay.b, overlay.a, base.b, base.a);

	return {r, g, b, out_a};
}

constexpr auto _laser_blended_colors = ([]() constexpr {
	std::array<sf::Color, _color_palette_size> arr{};
	for (unsigned int i = 0; i < _color_palette_size; ++i) {
		arr[i] = blendColor(colorOfName("Laser"), _colors[i].color);
	}
	return arr;
})();

constexpr sf::Color laserBlendedColorOfIndex(unsigned int index) {
	return _laser_blended_colors[index];
}

} // namespace wf

#endif // WFORGE_COLORPALETTE_H
