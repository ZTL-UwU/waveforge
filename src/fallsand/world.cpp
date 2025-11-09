#include "wforge/colorpalette.h"
#include "wforge/fallsand.h"
#include <memory>
#include <proxy/proxy.h>
#include <random>
#include <utility>

#ifndef NDEBUG
#include <cpptrace/cpptrace.hpp>
#include <format>
#include <iostream>
#endif

namespace wf {

PixelWorld::PixelWorld() noexcept: _width(0), _height(0) {}

PixelWorld::PixelWorld(int width, int height) noexcept
	: _width(width)
	, _height(height)
	, _tags(std::make_unique<PixelTag[]>(width * height))
	, _elements(std::make_unique<PixelElement[]>(width * height))
	, _fluid_cid(std::make_unique<int[]>(width * height)) {
	PixelTag airTag = element::Air().newTag();
	for (int i = 0; i < width * height; ++i) {
		_tags[i] = airTag;
		_elements[i] = pro::make_proxy_inplace<PixelFacade, element::Air>();
	}
}

PixelTag PixelWorld::tagOf(int x, int y) const noexcept {
#ifndef NDEBUG
	if (x < 0 || x >= _width || y < 0 || y >= _height) {
		std::cerr << std::format(
			"PixelWorld::tagOf: index out of bounds: x = {}, y = {}, width = "
			"{}, height = {}\n",
			x, y, _width, _height
		);
		cpptrace::generate_trace().print();
		std::abort();
	}
#endif
	return _tags[y * _width + x];
}

PixelTag &PixelWorld::tagOf(int x, int y) noexcept {
#ifndef NDEBUG
	if (x < 0 || x >= _width || y < 0 || y >= _height) {
		std::cerr << std::format(
			"PixelWorld::tagOf: index out of bounds: x = {}, y = {}, width = "
			"{}, height = {}\n",
			x, y, _width, _height
		);
		cpptrace::generate_trace().print();
		std::abort();
	}
#endif
	return _tags[y * _width + x];
}

PixelElement &PixelWorld::elementOf(int x, int y) noexcept {
#ifndef NDEBUG
	if (x < 0 || x >= _width || y < 0 || y >= _height) {
		std::cerr << std::format(
			"PixelWorld::elementOf: index out of bounds: x = {}, y = {}, width "
			"= "
			"{}, height = {}\n",
			x, y, _width, _height
		);
		cpptrace::generate_trace().print();
		std::abort();
	}
#endif
	return _elements[y * _width + x];
}

void PixelWorld::swapPixels(int x1, int y1, int x2, int y2) noexcept {
	// ADL two steps
	using std::swap;
	swap(tagOf(x1, y1), tagOf(x2, y2));
	swap(elementOf(x1, y1), elementOf(x2, y2));
}

void PixelWorld::replacePixel(int x, int y, PixelElement new_pixel) noexcept {
	tagOf(x, y) = new_pixel->newTag();
	elementOf(x, y) = std::move(new_pixel);
}

void PixelWorld::replacePixelWithAir(int x, int y) noexcept {
	replacePixel(x, y, pro::make_proxy_inplace<PixelFacade, element::Air>());
}

bool PixelWorld::typeOfIs(int x, int y, PixelType ptype) const noexcept {
	return tagOf(x, y).type == ptype;
}

bool PixelWorld::classOfIs(int x, int y, PixelClass pclass) const noexcept {
	return tagOf(x, y).pclass == pclass;
}

unsigned int PixelWorld::rand() const noexcept {
	// TODO: Replace with better RNG
	static std::mt19937 rng(998244353);
	return rng();
}

void PixelWorld::resetDirtyFlags() noexcept {
	for (int i = 0; i < _width * _height; ++i) {
		_tags[i].dirty = false;
	}
}

void PixelWorld::step() noexcept {
	fluidAnalysisStep();
	for (int y = _height - 1; y >= 0; --y) {
		bool reverse_x = this->rand() % 2 == 0;
		for (int ix = 0; ix < _width; ++ix) {
			int x = reverse_x ? (_width - 1 - ix) : ix;
			while (!tagOf(x, y).dirty) {
				tagOf(x, y).dirty = true;
				elementOf(x, y)->step(*this, x, y);
			}
		}
	}

	resetDirtyFlags();
}

void PixelWorld::renderToBuffer(std::span<std::uint8_t> buf) const noexcept {
#ifndef NDEBUG
	if (buf.size() != _width * _height * 4) {
		std::cerr << std::format(
			"PixelWorld::renderToTexture: buffer size mismatch: expected {}, "
			"got "
			"{}\n",
			_width * _height * 4, buf.size()
		);
		cpptrace::generate_trace().print();
		std::abort();
	}
#endif

	for (int i = 0; i < _width * _height; ++i) {
		auto color = colorOfIndex(_tags[i].color_index);
		buf[i * 4 + 0] = color.r;
		buf[i * 4 + 1] = color.g;
		buf[i * 4 + 2] = color.b;
		buf[i * 4 + 3] = color.a;
	}
}

} // namespace wf
