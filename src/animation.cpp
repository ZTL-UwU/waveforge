#include "wforge/assets.h"
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Vector2.hpp>
#include <format>
#include <stdexcept>

namespace wf {

PixelAnimationFrames::PixelAnimationFrames(
	sf::Texture &sprite_sheet, int frame_width, int frame_height
)
	: _texture(sprite_sheet)
	, _frame_width(frame_width)
	, _frame_height(frame_height) {
	int img_width = sprite_sheet.getSize().x;
	int img_height = sprite_sheet.getSize().y;

	if (img_height != frame_height) {
		throw std::invalid_argument(
			std::format(
				"PixelAnimationFrames: Sprite sheet height {} does not match "
				"frame height {}",
				img_height, frame_height
			)
		);
	}

	if (img_width % frame_width != 0) {
		throw std::invalid_argument(
			std::format(
				"PixelAnimationFrames: Sprite sheet width {} is not a multiple "
				"of frame width {}",
				img_width, frame_width
			)
		);
	}

	_length = img_width / frame_width;
	_frames.reserve(_length);
	for (int i = 0; i < _length; ++i) {
		_frames.emplace_back(
			sf::Vector2i(i * frame_width, 0),
			sf::Vector2i(frame_width, frame_height)
		);
	}
}

void PixelAnimationFrames::render(
	sf::RenderTarget &target, int frame_index, int x, int y, int scale
) const {
	if (frame_index < 0 || frame_index >= _length) {
		throw std::out_of_range(
			std::format(
				"PixelAnimationFrames::render: frame_index {} out of range [0, "
				"{})",
				frame_index, _length
			)
		);
	}

	sf::Sprite sprite(_texture, _frames[frame_index]);
	sprite.setPosition(sf::Vector2f(x * scale, y * scale));
	sprite.setScale(sf::Vector2f(scale, scale));
	target.draw(sprite);
}

} // namespace wf
