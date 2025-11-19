#include "wforge/2d.h"
#include "wforge/colorpalette.h"
#include "wforge/elements.h"
#include "wforge/fallsand.h"

namespace wf {
namespace element {

PixelTag Copper::newTag() const noexcept {
	return PixelTag{
		.type = PixelType::Copper,
		.pclass = PixelClass::Solid,
		.color_index = colorIndexOf("Copper1"),
		.thermal_conductivity = 60,
	};
}

PixelElement Copper::create() noexcept {
	return pro::make_proxy_inplace<PixelFacade, Copper>();
}

void Copper::onCharge(PixelWorld &world, int x, int y) noexcept {
	auto &tag = world.tagOf(x, y);
	if (tag.electric_power == 0) {
		tag.electric_power = PixelTag::electric_power_max;
	}
}

void Copper::step(PixelWorld &world, int x, int y) noexcept {
	auto my_tag = world.tagOf(x, y);
	if (my_tag.electric_power == PixelTag::electric_power_max - 1) {
		std::array<int, 2> world_dim{world.width(), world.height()};
		for (auto [nx, ny] : neighbors8({x, y}, world_dim)) {
			world.chargeElement(nx, ny);
		}
	}

	SolidElement::step(world, x, y);
}

} // namespace element
} // namespace wf
