#include "wforge/level.h"
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Vector2.hpp>
#include <cmath>

namespace wf {

Level::Level(int width, int height) noexcept: fallsand(width, height) {}

void Level::step() {
	fallsand.step();
	duck.step(*this);
	checkpoint.step(*this);
}

ItemStack *Level::activeItemStack() noexcept {
	if (_active_item_index < 0
	    || _active_item_index >= static_cast<int>(items.size())) {
		_active_item_index = -1;
		return nullptr;
	}

	if (items[_active_item_index].amount <= 0) {
		_active_item_index = -1;
		return nullptr;
	}

	return &items[_active_item_index];
}

void Level::useActiveItem(int x, int y, int scale) noexcept {
	if (auto itemstack = activeItemStack()) {
		if (itemstack->item->use(*this, x, y, scale)) {
			// item used successfully, decrease quantity
			auto &stack = items[_active_item_index];
			stack.amount -= 1;
			if (stack.amount <= 0) {
				_active_item_index = -1; // deactivate item
			}
		}
	}
}

void Level::changeActiveItemBrushSize(int delta) noexcept {
	if (auto itemstack = activeItemStack()) {
		itemstack->item->changeBrushSize(delta);
	}
}

void Level::selectItem(int index) noexcept {
	if (index < 0 || index >= static_cast<int>(items.size())) {
		_active_item_index = -1;
	} else {
		if (items[index].amount > 0) {
			_active_item_index = index;
		} else {
			_active_item_index = -1;
		}
	}
}

bool Level::isFailed() const noexcept {
	return duck.isOutOfWorld(*this);
}

bool Level::isCompleted() const noexcept {
	return checkpoint.isCompleted();
}

LevelRenderer::LevelRenderer(Level &level, int scale)
	: _level(level)
	, scale(scale)
	, _fallsand_buffer(
		  std::make_unique<std::uint8_t[]>(level.width() * level.height() * 4)
	  )
	, _fallsand_texture()
	, _fallsand_sprite(_fallsand_texture)
	, _duck_sprite(
		  AssetsManager::instance().getAsset<sf::Texture>("duck/texture")
	  ) {
	if (!_fallsand_texture.resize(
			sf::Vector2u(level.width(), level.height())
		)) {
		throw std::runtime_error("Failed to create fallsand texture");
	}
	_fallsand_texture.setSmooth(false);

	sf::Vector2f scale_vec(scale, scale);
	_fallsand_sprite = sf::Sprite(_fallsand_texture);
	_fallsand_sprite.setScale(scale_vec);
	_duck_sprite.setScale(scale_vec);
}

void LevelRenderer::_renderFallsand(sf::RenderTarget &target) noexcept {
	std::span<std::uint8_t> fallsand_buffer_view(
		_fallsand_buffer.get(), _level.width() * _level.height() * 4
	);
	_level.fallsand.renderToBuffer(fallsand_buffer_view);
	_fallsand_texture.update(_fallsand_buffer.get());
	target.draw(_fallsand_sprite);
}

void LevelRenderer::_renderDuck(sf::RenderTarget &target) noexcept {
	sf::Vector2f duck_pos(
		std::round(_level.duck.position.x) * scale,
		std::round(_level.duck.position.y) * scale
	);

	_duck_sprite.setPosition(duck_pos);
	target.draw(_duck_sprite);
}

void LevelRenderer::render(
	sf::RenderTarget &target, int mouse_x, int mouse_y
) noexcept {
	_renderFallsand(target);
	_renderDuck(target);
	_level.checkpoint.render(target, scale); // checkpoint can render itself

	if (auto itemstack = _level.activeItemStack()) {
		itemstack->item->render(target, mouse_x, mouse_y, scale);
	}
}

} // namespace wf
