#include "wforge/colorpalette.h"
#include "wforge/fallsand.h"
#include <cstddef>

namespace wf {
namespace element {

std::size_t Water::hash() const noexcept {
	const std::size_t magic = ('W' << 24) | ('A' << 16) | ('T' << 8) | 'R';
	return magic;
}

PixelTag Water::newTag() const noexcept {
	return PixelTag{
		.type = PixelType::Water,
		.pclass = PixelClass::Fluid,
		.color_index = colorIndexOf("Water"),
	};
}

const int water_dispersion_rate = 2;

void Water::step(PixelWorld &world, int x, int y) noexcept {
	if (y + 1 >= world.height()) {
		world.replacePixelWithAir(x, y);
		return;
	}

	auto &my_tag = world.tagOf(x, y);

	auto below_tag = world.tagOf(x, y + 1);
	if (below_tag.type == PixelType::Air) {
		my_tag.is_free_falling = true;
		world.swapPixels(x, y, x, y + 1);
		return;
	}
	my_tag.is_free_falling = false;

	if (my_tag.fluid_dir == 0) {
		my_tag.fluid_dir = (world.rand() % 2 == 0) ? 1 : -1;
	}

	for (auto d : {my_tag.fluid_dir, -my_tag.fluid_dir}) {
		int new_x = x + d;
		if (new_x < 0 || new_x >= world.width()) {
			world.replacePixelWithAir(x, y);
			return;
		}

		auto diag_tag = world.tagOf(new_x, y + 1);
		if (diag_tag.pclass == PixelClass::Gas) {
			my_tag.fluid_dir = d;
			world.swapPixels(x, y, new_x, y + 1);
			break;
		}

		auto side_tag = world.tagOf(new_x, y);
		if (side_tag.pclass == PixelClass::Gas) {
			my_tag.fluid_dir = d;
			world.swapPixels(x, y, new_x, y);
			break;
		}
	}
	my_tag.fluid_dir = 0;
}

} // namespace element
} // namespace wf