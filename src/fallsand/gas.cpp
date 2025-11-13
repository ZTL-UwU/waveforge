#include "wforge/fallsand.h"
#include "wforge/xoroshiro.h"

namespace wf {
namespace element {

namespace {

constexpr int gas_dispersion_rate = 4;
constexpr int gas_go_diag_chance = 50; // %

bool canSwapTag(PixelTag target, PixelTag self) {
	return target.pclass == PixelClass::Fluid
		|| (target.pclass == PixelClass::Gas
	        && isDenser(target.type, self.type));
}

} // namespace

void GasElement::step(PixelWorld &world, int x, int y) noexcept {
	if (y == 0) {
		world.replacePixelWithAir(x, y);
		return;
	}

	auto &rng = Xoroshiro128PP::globalInstance();
	auto &my_tag = world.tagOf(x, y);

	// Try to move diagonally up
	int dir = (rng() % 2) * 2 - 1; // -1 or +1

	if (x + dir >= 0 && x + dir < world.width()
	    && (rng() % 100 < gas_go_diag_chance)) {
		auto step1_diag_tag = world.tagOf(x + dir, y - 1);
		if (canSwapTag(step1_diag_tag, my_tag)) {
			world.swapPixels(x, y, x + dir, y - 1);
			return;
		}
	}

	// Try to move up
	auto above_tag = world.tagOf(x, y - 1);
	if (canSwapTag(above_tag, my_tag)) {
		world.swapPixels(x, y, x, y - 1);
		return;
	}

	int to_x = x, to_y = y;
	for (int i = 1; i <= gas_dispersion_rate; ++i) {
		int dx = dir * i;
		if (x + dx < 0 || x + dx >= world.width()) {
			world.replacePixelWithAir(x, y);
			return;
		}

		auto diag_tag = world.tagOf(x + dx, y - 1);
		if (canSwapTag(diag_tag, my_tag)) {
			to_x = x + dx;
			to_y = y - 1;
			break;
		}

		auto side_tag = world.tagOf(x + dx, y);
		if (canSwapTag(side_tag, my_tag)) {
			to_x = x + dx;
			to_y = y;
		}
	}

	if (to_x != x || to_y != y) {
		world.swapPixels(x, y, to_x, to_y);
		return;
	}
}

} // namespace element
} // namespace wf
