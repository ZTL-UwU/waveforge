#include "wforge/colorpalette.h"
#include "wforge/elements.h"
#include "wforge/fallsand.h"
#include "wforge/xoroshiro.h"
#include <proxy/v4/proxy.h>

namespace wf {
namespace element {

PixelTag Oil::newTag() const noexcept {
	return PixelTag{
		.type = PixelType::Oil,
		.pclass = PixelClass::Fluid,
		.color_index = colorIndexOf("Oil"),
		.thermal_conductivity = 28,
	};
}

void Oil::step(PixelWorld &world, int x, int y) noexcept {
	constexpr unsigned int oil_ignite_heat_threshold = 40;
	constexpr unsigned int produced_fire_heat = 50;
	constexpr unsigned int oil_burn_duration = 48;
	constexpr unsigned int smoke_heat = 50;
	constexpr unsigned int die_smoke_chance = 25;   // %
	constexpr unsigned int random_smoke_chance = 3; // %

	auto &my_tag = world.tagOf(x, y);
	if (my_tag.heat >= oil_ignite_heat_threshold) {
		my_tag.ignited = true;
	}

	if (my_tag.ignited && my_tag.heat < oil_ignite_heat_threshold) {
		my_tag.ignited = false;
	}

	if (my_tag.ignited) {
		burn_time += 1;
		int next_heat = std::min(
			PixelTag::heat_max, my_tag.heat + produced_fire_heat
		);

		auto &rng = Xoroshiro128PP::globalInstance();
		if (burn_time >= oil_burn_duration) {
			if (rng.next() % 100 < die_smoke_chance) {
				world.replacePixel(x, y, pro::make_proxy<PixelFacade, Smoke>());
			} else {
				world.replacePixelWithAir(x, y);
			}
			my_tag.heat = next_heat;
			return;
		}
		my_tag.heat = next_heat;

		if (y > 0) {
			auto &above_tag = world.tagOf(x, y - 1);
			if (above_tag.type == PixelType::Air) {
				if (rng.next() % 100 < random_smoke_chance) {
					world.replacePixel(
						x, y - 1, pro::make_proxy<PixelFacade, Smoke>()
					);
					above_tag.heat = smoke_heat;
				}
			}
		}
	}
	FluidElement::step(world, x, y);
}

} // namespace element
} // namespace wf
