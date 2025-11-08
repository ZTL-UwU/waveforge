#ifndef WFORGE_LEVEL_H
#define WFORGE_LEVEL_H

#include "wforge/assets.h"
#include "wforge/fallsand.h"
#include <SFML/System/Vector2.hpp>
#include <memory>

namespace wf {

class Level;

struct DuckEntity {
	DuckEntity(sf::Vector2f pos = {.0f, .0f}) noexcept;

	PixelShape shape;
	sf::Vector2f position; // anchor at top-left
	sf::Vector2f velocity;

	bool isOutOfWorld(const Level &level) const noexcept;
	bool willCollideAt(
		const Level &level, int target_x, int target_y
	) const noexcept;
	bool currentlyColliding(const Level &level) const noexcept;

	void step(const Level &level) noexcept;
};

struct GoalArea {
	// Anchor at top-left
	int x;
	int y;
	int progress;

	GoalArea();
	GoalArea(int x, int y);

	int width() const noexcept {
		return _width;
	}

	int height() const noexcept {
		return _height;
	}

	void step(const Level &level) noexcept;
	void render(sf::RenderTarget &target, int scale) const noexcept;

	bool isCompleted() const noexcept;

private:
	int _width, _height;
	GoalSprite &_sprite;

	bool _isDuckInside(const Level &level) const noexcept;
};

struct Level {
	Level(int width, int height) noexcept;

	PixelWorld fallsand;
	DuckEntity duck; // Quack!
	GoalArea goal;

	auto width() const noexcept {
		return fallsand.width();
	}

	auto height() const noexcept {
		return fallsand.height();
	}

	bool isFailed() const noexcept;
	bool isCompleted() const noexcept;

	void step();
};

struct LevelRenderer {
	LevelRenderer(Level &level, int scale);

	int scale;

	void render(sf::RenderTarget &target) noexcept;

private:
	Level &_level;
	std::unique_ptr<std::uint8_t[]> _fallsand_buffer;
	sf::Texture _fallsand_texture;
	sf::Sprite _fallsand_sprite;
	sf::Sprite _duck_sprite;

	void _renderFallsand(sf::RenderTarget &target) noexcept;
	void _renderDuck(sf::RenderTarget &target) noexcept;
};

} // namespace wf

#endif // WFORGE_LEVEL_H
