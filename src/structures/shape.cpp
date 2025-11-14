#include "wforge/elements.h"
#include "wforge/structures.h"
#include <memory>

#ifndef NDEBUG
#include <cpptrace/cpptrace.hpp>
#include <format>
#include <iostream>
#endif

namespace wf {
namespace structure {

PixelShapedStructure::PixelShapedStructure(
	int x, int y, const PixelShape &shape
) noexcept
	: PositionedStructure(x, y)
	, _shape(shape)
	, _pixel_types(
		  std::make_unique<PixelType[]>(shape.width() * shape.height())
	  ) {
	for (int sy = 0; sy < shape.height(); ++sy) {
		for (int sx = 0; sx < shape.width(); ++sx) {
			_pixel_types[sy * shape.width() + sx] = shape.pixelTypeOf(sx, sy);
			if (shape.isPOIPixel(sx, sy)) {
				poi.push_back({sx, sy});
			}
		}
	}
}

void PixelShapedStructure::setup(PixelWorld &world) noexcept {
	for (int sy = 0; sy < height(); ++sy) {
		for (int sx = 0; sx < width(); ++sx) {
			int wx = x + sx;
			int wy = y + sy;
			auto ptype = _pixel_types[sy * width() + sx];

			switch (ptype) {
			case PixelType::Air:
				break;

			case PixelType::Stone:
				world.replacePixel(wx, wy, element::Stone::create());
				break;

			case PixelType::Wood:
				world.replacePixel(wx, wy, element::Wood::create());
				break;

			case PixelType::Copper:
				world.replacePixel(wx, wy, element::Copper::create());
				break;

			case PixelType::Sand:
				world.replacePixel(wx, wy, element::Sand::create());
				break;

			default:
				world.replacePixel(wx, wy, element::Decoration::create());
				break;
			}
		}
	}
}

PixelType PixelShapedStructure::pixelTypeOf(int px, int py) const noexcept {
#ifndef NDEBUG
	if (px < 0 || px >= width() || py < 0 || py >= height()) {
		std::cerr << std::format(
			"PixelShapedStructure::pixelTypeOf: out of bounds access at ({}, "
			"{}) "
			"for size ({}, {})\n",
			px, py, width(), height()
		);
		cpptrace::generate_trace().print();
		std::abort();
	}
#endif

	return _pixel_types[py * width() + px];
}

void PixelShapedStructure::customRender(
	std::span<std::uint8_t> buf, const PixelWorld &world
) const noexcept {
	for (int sy = 0; sy < height(); ++sy) {
		for (int sx = 0; sx < width(); ++sx) {
			int world_x = x + sx;
			int world_y = y + sy;
			if (_pixel_types[sy * width() + sx] != PixelType::Decoration) {
				continue;
			}

			int buf_index = (world_y * world.width() + world_x) * 4;
			sf::Color color = _shape.colorOf(sx, sy);
			buf[buf_index + 0] = color.r;
			buf[buf_index + 1] = color.g;
			buf[buf_index + 2] = color.b;
			buf[buf_index + 3] = color.a;
		}
	}
}

bool PixelShapedStructure::step(PixelWorld &world) const noexcept {
	for (int sy = 0; sy < height(); ++sy) {
		for (int sx = 0; sx < width(); ++sx) {
			int world_x = x + sx;
			int world_y = y + sy;
			PixelType expected_type = _pixel_types[sy * width() + sx];
			if (expected_type == PixelType::Air) {
				continue;
			}

			PixelTag tag = world.tagOf(world_x, world_y);
			if (tag.type != expected_type) {
				return false;
			}
		}
	}

	return true;
}

} // namespace structure
} // namespace wf
