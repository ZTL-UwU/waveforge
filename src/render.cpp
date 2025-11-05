#include "wforge/render.h"
#include "wforge/colorpalette.h"
#include "wforge/fallsand.h"
#include <SFML/Graphics.hpp>
#include <cstdio>
#include <string>

namespace wf {

Renderer::Renderer(int pixelScale) noexcept: _scale(pixelScale) {}

bool Renderer::init(int width, int height) {
	if (width <= 0 || height <= 0) {
		return false;
	}
	_width = width;
	_height = height;
	_pixels.assign(static_cast<size_t>(_width) * _height * 4, 0);

	// create / resize texture sized to world pixels (SFML 3 API)
	if (!_texture.resize(
			sf::Vector2u(
				static_cast<unsigned int>(_width),
				static_cast<unsigned int>(_height)
			)
		)) {
		return false;
	}
	_texture.setSmooth(false);

	// create sprite bound to the texture (SFML 3 requires a texture at
	// construction)
	_sprite = std::make_unique<sf::Sprite>(_texture);
	_sprite->setScale({static_cast<float>(_scale), static_cast<float>(_scale)});
	return true;
}

void Renderer::uploadFromWorld(const PixelWorld &world) noexcept {
	if (world.width() != _width || world.height() != _height) {
		// re-init on size change
		init(world.width(), world.height());
	}

	// fill RGBA buffer from world tags
	for (int y = 0; y < _height; ++y) {
		for (int x = 0; x < _width; ++x) {
			auto tag = world.tagOf(x, y);
			// clamp color index to palette
			unsigned int idx = tag.color_index;
			if (idx >= static_cast<unsigned int>(_color_palette_size)) {
				idx = 0;
			}
			auto c = colorOfIndex(idx);

			size_t off = 4 * (static_cast<size_t>(y) * _width + x);
			_pixels[off + 0] = c.r;
			_pixels[off + 1] = c.g;
			_pixels[off + 2] = c.b;
			_pixels[off + 3] = c.a;
		}
	}

	// upload pixels to texture
	// sf::Texture::update supports raw pixels in RGBA order
	_texture.update(_pixels.data());
}

void Renderer::draw(sf::RenderTarget &target) noexcept {
	if (_sprite) {
		target.draw(*_sprite);
	}
}

bool Renderer::saveFrame(const std::string &path) const noexcept {
	try {
		auto img = _texture.copyToImage();
		return img.saveToFile(path);
	} catch (...) {
		return false;
	}
}

} // namespace wf
