#ifndef WFORGE_LEVEL_H
#define WFORGE_LEVEL_H

#include "wforge/assets.h"
#include "wforge/fallsand.h"
#include <SFML/System/Vector2.hpp>

namespace wf {

class Level;

struct DuckEntity {
	DuckEntity(sf::Vector2f pos = {.0f, .0f}) noexcept;

	PixelShape shape;
	sf::Vector2f position; // anchor at top-left
	sf::Vector2f velocity;

	void step(const Level &level) noexcept;
};

struct Level {
	Level(int width, int height) noexcept;

	PixelWorld fallsand;
	DuckEntity duck; // Quack!

	auto width() const noexcept {
		return fallsand.width();
	}

	auto height() const noexcept {
		return fallsand.height();
	}

	void step();
};

} // namespace wf

#endif // WFORGE_LEVEL_H
