#include "wforge/assets.h"
#include "wforge/level.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Vector2.hpp>
#include <cmath>
#include <cpptrace/basic.hpp>
#include <cstdlib>

#ifndef NDEBUG
#include <cpptrace/cpptrace.hpp>
#include <format>
#include <iostream>
#endif

namespace wf {

GoalArea::GoalArea(): GoalArea(0, 0) {}

GoalArea::GoalArea(int x, int y)
	: x(x)
	, y(y)
	, progress(0)
	, _sprite(AssetsManager::instance().getAsset<GoalSprite>("goal/sprite")) {
	_width = _sprite.width();
	_height = _sprite.height();
}

void GoalArea::step(const Level &level) noexcept {
	if (_isDuckInside(level)) {
		progress = std::min(progress + 1, _height);
	} else {
		progress = std::max(progress - 1, 0);
	}
}

bool GoalArea::isCompleted() const noexcept {
	return progress >= _height;
}

void GoalArea::render(sf::RenderTarget &target, int scale) const noexcept {
	_sprite.render(target, x, y, progress, scale);
}

bool GoalArea::_isDuckInside(const Level &level) const noexcept {
	// check if every pixel of duck shape is inside goal area

	const auto &duck = level.duck;
	int duck_x = std::round(duck.position.x);
	int duck_y = std::round(duck.position.y);

	// Fast AABB check
	if (duck_x + duck.shape.width() <= x || duck_x >= x + _width) {
		return false;
	}

	if (duck_y + duck.shape.height() <= y || duck_y >= y + _height) {
		return false;
	}

	// Pixel-perfect check
	auto &shape = duck.shape;
	for (int dx = 0; dx < shape.width(); ++dx) {
		for (int dy = 0; dy < shape.height(); ++dy) {
			if (!shape.hasPixel(dx, dy)) {
				continue;
			}
			int wx = duck_x + dx;
			int wy = duck_y + dy;

			if (wx < x || wx >= x + _width || wy < y || wy >= y + _height) {
				return false;
			}
		}
	}
	return true;
}

GoalSprite::GoalSprite(sf::Image &goal_1, sf::Image &goal_2) {
#ifndef NDEBUG
	if (goal_1.getSize() != goal_2.getSize()) {
		std::cerr << std::format(
			"wf::GoalSprite: goal_1 and goal_2 have different sizes: ({}, {}) "
			"vs ({}, {})\n",
			goal_1.getSize().x, goal_1.getSize().y, goal_2.getSize().x,
			goal_2.getSize().y
		);
		cpptrace::generate_trace().print();
		std::abort();
	}
#endif

	if (!_goal_1.loadFromImage(goal_1)) {
#ifndef NDEBUG
		std::cerr << "wf::GoalSprite: failed to load goal_1 texture\n";
		cpptrace::generate_trace().print();
#endif
		std::abort();
	}

	if (!_goal_2.loadFromImage(goal_2)) {
#ifndef NDEBUG
		std::cerr << "wf::GoalSprite: failed to load goal_2 texture\n";
		cpptrace::generate_trace().print();
#endif
		std::abort();
	}

	_goal_1.setSmooth(false);
	_goal_2.setSmooth(false);
}

int GoalSprite::width() const noexcept {
	return _goal_1.getSize().x;
}

int GoalSprite::height() const noexcept {
	return _goal_1.getSize().y;
}

void GoalSprite::render(
	sf::RenderTarget &target, int x, int y, int progress, int scale
) const noexcept {
	// Top height - progress pixels from goal_1
	// Bottom progress pixels from goal_2
	auto height = this->height();
	auto width = this->width();

#ifndef NDEBUG
	if (progress > height || progress < 0) {
		std::cerr << std::format(
			"wf::GoalSprite::render: progress {} out of bounds [0, {}]\n",
			progress, height
		);
		cpptrace::generate_trace().print();
		std::abort();
	}
#endif

	if (progress > 0) {
		sf::Sprite sprite_bottom(
			_goal_2, sf::IntRect({0, height - progress}, {width, progress})
		);

		sprite_bottom.setPosition(
			sf::Vector2f(x * scale, (y + height - progress) * scale)
		);
		sprite_bottom.setScale(sf::Vector2f(scale, scale));
		target.draw(sprite_bottom);
	}

	if (progress < height) {
		sf::Sprite sprite_top(
			_goal_1, sf::IntRect({0, 0}, {width, height - progress})
		);

		sprite_top.setPosition(sf::Vector2f(x * scale, y * scale));
		sprite_top.setScale(sf::Vector2f(scale, scale));
		target.draw(sprite_top);
	}
}

} // namespace wf
