#include "wforge/colorpalette.h"
#include "wforge/fallsand.h"
#include <cstddef>

namespace wf {
namespace element {

std::size_t Water::hash() const noexcept {
	const std::size_t magic = ('W' << 24) | ('A' << 16) | ('T' << 8) | 'R';
	return magic ^ static_cast<std::size_t>(dir);
}

PixelTag Water::defaultTag() const noexcept {
	return PixelTag{
		.type = PixelType::Water,
		.pclass = PixelClass::Fluid,
		.color_index = colorIndexOf("Water"),
	};
}

void Water::step(PixelWorld &world, int x, int y) noexcept {
	if (y + 1 >= world.height()) {
		world.replacePixelWithAir(x, y);
		return;
	}

	auto below_tag = world.tagOf(x, y + 1);
	if (below_tag.type == PixelType::Air) {
		dir = 0;
		world.swapPixels(x, y, x, y + 1);
		return;
	}

	if (dir == 0) {
		dir = (world.rand() % 2) * 2 - 1; // -1 or +1
	}

	for (auto d : {dir, -dir}) {
		int new_x = x + d;
		if (new_x < 0 || new_x >= world.width()) {
			world.replacePixelWithAir(x, y);
			return;
		}

		auto diag_tag = world.tagOf(new_x, y + 1);
		if (diag_tag.pclass == PixelClass::Gas) {
			dir = d;
			world.swapPixels(x, y, new_x, y + 1);
			return;
		}

		auto side_tag = world.tagOf(new_x, y);
		if (side_tag.pclass == PixelClass::Gas) {
			dir = d;
			world.swapPixels(x, y, new_x, y);
			return;
		}
	}
}

} // namespace element
} // namespace wf