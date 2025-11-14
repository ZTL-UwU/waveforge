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

namespace {

constexpr int _ticks_per_progress = 3;

}

CheckpointArea::CheckpointArea(): CheckpointArea(0, 0) {}

CheckpointArea::CheckpointArea(int x, int y)
	: x(x)
	, y(y)
	, _progress(0)
	, _sprite(
		  AssetsManager::instance().getAsset<CheckpointSprite>(
			  "checkpoint/sprite"
		  )
	  ) {
	_width = _sprite.width();
	_height = _sprite.height();
}

void CheckpointArea::step(const Level &level) noexcept {
	if (_isDuckInside(level)) {
		_progress = std::min(_progress + 1, _height * _ticks_per_progress);
	} else {
		_progress = std::max(0, _progress - 1);
	}
}

void CheckpointArea::resetProgress() noexcept {
	_progress = 0;
}

int CheckpointArea::progress() const noexcept {
	return _progress / _ticks_per_progress;
}

bool CheckpointArea::isCompleted() const noexcept {
	return _progress >= _height * _ticks_per_progress;
}

void CheckpointArea::render(
	sf::RenderTarget &target, int scale
) const noexcept {
	_sprite.render(target, x, y, progress(), scale);
}

bool CheckpointArea::_isDuckInside(const Level &level) const noexcept {
	// check if any pixel of duck shape is inside checkpoint area

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

			if (wx >= x && wx < x + _width && wy >= y && wy < y + _height) {
				return true;
			}
		}
	}
	return false;
}

CheckpointSprite::CheckpointSprite(
	sf::Image &checkpoint_1, sf::Image &checkpoint_2
) {
#ifndef NDEBUG
	if (checkpoint_1.getSize() != checkpoint_2.getSize()) {
		std::cerr << std::format(
			"{}: checkpoint_1 and checkpoint_2 have different sizes: ({}, {}) "
			"vs ({}, {})\n",
			__FUNCTION__, checkpoint_1.getSize().x, checkpoint_1.getSize().y,
			checkpoint_2.getSize().x, checkpoint_2.getSize().y
		);
		cpptrace::generate_trace().print();
		std::abort();
	}
#endif

	if (!_ckeckpoint_1.loadFromImage(checkpoint_1)) {
#ifndef NDEBUG
		std::cerr << "Failed to load checkpoint_1 texture\n";
		cpptrace::generate_trace().print();
#endif
		std::abort();
	}

	if (!_checkpoint_2.loadFromImage(checkpoint_2)) {
#ifndef NDEBUG
		std::cerr << "Failed to load checkpoint_2 texture\n";
		cpptrace::generate_trace().print();
#endif
		std::abort();
	}

	_ckeckpoint_1.setSmooth(false);
	_checkpoint_2.setSmooth(false);
}

int CheckpointSprite::width() const noexcept {
	return _ckeckpoint_1.getSize().x;
}

int CheckpointSprite::height() const noexcept {
	return _ckeckpoint_1.getSize().y;
}

void CheckpointSprite::render(
	sf::RenderTarget &target, int x, int y, int progress, int scale
) const noexcept {
	// Top height - progress pixels from checkpoint_1
	// Bottom progress pixels from checkpoint_2
	auto height = this->height();
	auto width = this->width();

#ifndef NDEBUG
	if (progress > height || progress < 0) {
		std::cerr << std::format(
			"wf::CheckpointSprite::render: progress {} out of bounds [0, {}]\n",
			progress, height
		);
		cpptrace::generate_trace().print();
		std::abort();
	}
#endif

	if (progress > 0) {
		sf::Sprite sprite_bottom(
			_checkpoint_2,
			sf::IntRect({0, height - progress}, {width, progress})
		);

		sprite_bottom.setPosition(
			sf::Vector2f(x * scale, (y + height - progress) * scale)
		);
		sprite_bottom.setScale(sf::Vector2f(scale, scale));
		target.draw(sprite_bottom);
	}

	if (progress < height) {
		sf::Sprite sprite_top(
			_ckeckpoint_1, sf::IntRect({0, 0}, {width, height - progress})
		);

		sprite_top.setPosition(sf::Vector2f(x * scale, y * scale));
		sprite_top.setScale(sf::Vector2f(scale, scale));
		target.draw(sprite_top);
	}
}

} // namespace wf
