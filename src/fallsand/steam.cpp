#include "wforge/colorpalette.h"
#include "wforge/fallsand.h"
#include "wforge/xoroshiro.h"

namespace wf {
namespace element {

PixelTag Steam::newTag() const noexcept {
	auto rng = Xoroshiro128PP::globalInstance().next() % 3;
	return PixelTag{
		.type = PixelType::Steam,
		.pclass = PixelClass::Gas,
		.color_index = rng ? colorIndexOf("Steam1") : colorIndexOf("Steam2"),
		.is_free_falling = true,
		.thermal_conductivity = 4,
	};
}

void Steam::step(PixelWorld &world, int x, int y) noexcept {
	constexpr int steam_condensation_heat_threshold = 10;

	auto &my_tag = world.tagOf(x, y);
	if (my_tag.heat <= steam_condensation_heat_threshold) {
		int old_heat = my_tag.heat;
		world.replacePixel(x, y, pro::make_proxy<PixelFacade, Water>());
		my_tag.heat = old_heat;
		return;
	}
	GasElement::step(world, x, y);
}

} // namespace element
} // namespace wf
