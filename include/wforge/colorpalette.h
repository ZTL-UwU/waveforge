#ifndef WFORGE_COLORPALETTE_H
#define WFORGE_COLORPALETTE_H

#include <SFML/Graphics/Color.hpp>

namespace wf {

struct ColorPaletteEntry {
	const char *name;
	sf::Color color; // in RGBA
};

constexpr ColorPaletteEntry _colors[] = {
	{
		.name = "Air",
		.color = sf::Color(0, 0, 0, 0),
	},
	{
		.name = "Stone1",
		.color = sf::Color(128, 128, 128, 255),
	},
	{
		.name = "Sand",
		.color = sf::Color(194, 178, 128, 255),
	},
	{
		.name = "Water",
		.color = sf::Color(64, 164, 223, 200),
	},
	{
		.name = "DebugRed",
		.color = sf::Color(255, 0, 0, 255),
	}
};

constexpr int _color_palette_size = sizeof(_colors) / sizeof(ColorPaletteEntry);

inline consteval unsigned int colorIndexOf(const char *name) {
	for (unsigned int i = 0; i < _color_palette_size; ++i) {
		auto j = _colors[i].name;
		auto k = name;
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

} // namespace wf

#endif // WFORGE_COLORPALETTE_H
