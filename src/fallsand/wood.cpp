#include "wforge/colorpalette.h"
#include "wforge/fallsand.h"
#include "wforge/xoroshiro.h"

namespace wf {
namespace element {

PixelTag Wood::newTag() const noexcept {
	return PixelTag{
		.type = PixelType::Wood,
		.pclass = PixelClass::Solid,
		.color_index = colorIndexOf("Wood"),
		.thermal_conductivity = 20,
	};
}

void Wood::step(PixelWorld &world, int x, int y) noexcept {
	constexpr unsigned int wood_ignition_heat_threshold = 60;
	constexpr unsigned int produced_fire_heat = 40;
	constexpr unsigned int wood_burn_duration = 96;
	constexpr unsigned int smoke_heat = 40;
	constexpr unsigned int die_smoke_chance = 25;   // %
	constexpr unsigned int random_smoke_chance = 2; // %

	auto &my_tag = world.tagOf(x, y);
	if (!my_tag.ignited && my_tag.heat >= wood_ignition_heat_threshold) {
		my_tag.ignited = true;
	}

	if (my_tag.ignited) {
		burn_time += 1;
		my_tag.heat = std::min(
			PixelTag::heat_max, my_tag.heat + produced_fire_heat
		);

		auto &rng = Xoroshiro128PP::globalInstance();
		if (burn_time >= wood_burn_duration) {
			if (rng.next() % 100 < die_smoke_chance) {
				world.replacePixel(x, y, pro::make_proxy<PixelFacade, Smoke>());
			} else {
				world.replacePixelWithAir(x, y);
			}
			return;
		}

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

	SolidElement::step(world, x, y);
}

} // namespace element
} // namespace wf