#include "wforge/colorpalette.h"
#include "wforge/fallsand.h"

namespace wf {
namespace element {

std::size_t Sand::hash() const noexcept {
	// Some arbitrary hash value for sand
	return ('S' << 24) | ('A' << 16) | ('N' << 8) | 'D';
}

PixelTag Sand::defaultTag() const noexcept {
	return PixelTag{
		.type = PixelType::Sand,
		.pclass = PixelClass::Solid,
		.color_index = colorIndexOf("Sand"),
	};
}

bool isSwappaleTag(PixelTag tag) noexcept {
	return tag.type == PixelType::Air || tag.pclass == PixelClass::Fluid
		|| tag.pclass == PixelClass::Particle;
}

void Sand::step(PixelWorld &world, int x, int y) noexcept {
	if (y + 1 >= world.height()) {
		world.replacePixelWithAir(x, y);
		return;
	}

	auto below_tag = world.tagOf(x, y + 1);
	if (isSwappaleTag(below_tag)) {
		world.swapPixels(x, y, x, y + 1);
		return;
	}

	int firstDir = (world.rand() % 2) * 2 - 1; // -1 or +1
	for (auto dir : {firstDir, -firstDir}) {
		int new_x = x + dir;
		if (new_x < 0 || new_x >= world.width()) {
			world.replacePixelWithAir(x, y);
			return;
		}

		auto diag_tag = world.tagOf(new_x, y + 1);
		if (isSwappaleTag(diag_tag)) {
			world.swapPixels(x, y, new_x, y + 1);
			return;
		}
	}
}

} // namespace element
} // namespace wf