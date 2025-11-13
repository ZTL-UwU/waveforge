#include "wforge/colorpalette.h"
#include "wforge/fallsand.h"
#include "wforge/xoroshiro.h"

namespace wf {
namespace element {

PixelTag Smoke::newTag() const noexcept {
	auto rand = Xoroshiro128PP::globalInstance().next() % 3;
	return PixelTag{
		.type = PixelType::Smoke,
		.pclass = PixelClass::Gas,
		.color_index = rand ? colorIndexOf("Smoke1") : colorIndexOf("Smoke2"),
		.is_free_falling = true,
		.thermal_conductivity = 5,
	};
}

void Smoke::step(PixelWorld &world, int x, int y) noexcept {
	constexpr int smoke_disappear_heat_threshold = 12;
	auto &my_tag = world.tagOf(x, y);
	if (my_tag.heat <= smoke_disappear_heat_threshold) {
		int old_heat = my_tag.heat;
		world.replacePixelWithAir(x, y);
		my_tag.heat = old_heat;
		return;
	}
	GasElement::step(world, x, y);
}

} // namespace element
} // namespace wf