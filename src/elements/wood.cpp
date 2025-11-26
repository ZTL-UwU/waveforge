#include "wforge/2d.h"
#include "wforge/colorpalette.h"
#include "wforge/elements.h"
#include "wforge/fallsand.h"
#include "wforge/xoroshiro.h"
#include <random>

namespace wf::element {

Wood::Wood() noexcept {
	constexpr unsigned int burn_duration = 96;
	constexpr unsigned int burn_dur_variance = 24;

	auto &rng = Xoroshiro128PP::globalInstance();
	std::binomial_distribution<> burn_dur_dist(burn_dur_variance * 2, 0.5);
	burn_time_left = burn_duration - burn_dur_variance + burn_dur_dist(rng);
}

PixelTag Wood::newTag() const noexcept {
	auto color_rd = Xoroshiro128PP::globalInstance().next() % 3;
	return PixelTag{
		.type = PixelType::Wood,
		.pclass = PixelClass::Solid,
		.color_index = color_rd ? colorIndexOf("Wood1") : colorIndexOf("Wood2"),
		.thermal_conductivity = 20,
	};
}

PixelElement Wood::create() noexcept {
	return pro::make_proxy_inplace<PixelFacade, Wood>();
}

void Wood::step(PixelWorld &world, int x, int y) noexcept {
	constexpr unsigned int ignition_heat_threshold = 60;
	constexpr unsigned int produced_fire_heat = 40;
	constexpr unsigned int produced_fire_heat_to_neighbors = 2;
	constexpr unsigned int smoke_heat = 40;
	constexpr unsigned int die_smoke_chance = 25;   // %
	constexpr unsigned int random_smoke_chance = 2; // %
	constexpr unsigned int ignition_chance = 10;    // %

	auto &rng = Xoroshiro128PP::globalInstance();
	auto &my_tag = world.tagOf(x, y);
	if (!my_tag.ignited && my_tag.heat >= ignition_heat_threshold) {
		if (rng.next() % 100 < ignition_chance) {
			my_tag.ignited = true;
		}
	}

	if (my_tag.ignited) {
		burn_time_left -= 1;
		my_tag.heat = std::min(
			PixelTag::heat_max, my_tag.heat + produced_fire_heat
		);

		for (auto [nx, ny] :
		     neighbors4({x, y}, {world.width(), world.height()})) {
			auto &neighbor_tag = world.tagOf(nx, ny);
			neighbor_tag.heat = std::min(
				PixelTag::heat_max,
				neighbor_tag.heat + produced_fire_heat_to_neighbors
			);
		}

		if (burn_time_left <= 0) {
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

} // namespace wf::element
