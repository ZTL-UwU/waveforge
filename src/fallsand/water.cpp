#include "wforge/colorpalette.h"
#include "wforge/elements.h"
#include "wforge/fallsand.h"

namespace wf {
namespace element {

PixelTag Water::newTag() const noexcept {
	return PixelTag{
		.type = PixelType::Water,
		.pclass = PixelClass::Fluid,
		.color_index = colorIndexOf("Water"),
		.thermal_conductivity = 24,
	};
}

void Water::step(PixelWorld &world, int x, int y) noexcept {
	constexpr int water_vaporization_heat_threshold = 30;

	auto &my_tag = world.tagOf(x, y);
	if (my_tag.heat >= water_vaporization_heat_threshold) {
		int old_heat = my_tag.heat;
		world.replacePixel(x, y, pro::make_proxy<PixelFacade, Steam>());
		my_tag.heat = old_heat;
		return;
	}

	FluidElement::step(world, x, y);
}

} // namespace element
} // namespace wf
