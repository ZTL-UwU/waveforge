#include "wforge/2d.h"
#include "wforge/fallsand.h"
#include "wforge/xoroshiro.h"
#include <cmath>
#include <type_traits>
#include <vector>

namespace wf {

namespace {

constexpr float heat_transfer_factor = 0.15f;
constexpr float heat_decay_factor = 0.005f;

} // namespace

void PixelWorld::thermalAnalysisStep() noexcept {
	auto &rng = Xoroshiro128PP::globalInstance();
	std::array<int, 2> world_dim{_width, _height};

	std::vector<unsigned int> next_heat(_width * _height, 0);

	// Heat transfer
	for (int y = 0; y < _height; ++y) {
		for (int x = 0; x < _width; ++x) {
			auto &tag = tagOf(x, y);

			if (tag.heat == 0 || tag.thermal_conductivity == 0) {
				next_heat[y * _width + x] += tag.heat;
				continue;
			}

			int transfer_amount = 0;

			int total_thermal_conductivity = std::round(
				tag.heat
				* (PixelTag::thermal_conductivity_max
			       - tag.thermal_conductivity)
				/ heat_transfer_factor
			);
			for (auto [nx, ny] : neighborsOf({x, y}, world_dim)) {
				auto &ntag = tagOf(nx, ny);
				total_thermal_conductivity += std::max<int>(
												  0, tag.heat - ntag.heat
											  )
					* std::min(tag.thermal_conductivity,
				               ntag.thermal_conductivity);
			}

			for (auto [nx, ny] : neighborsOf({x, y}, world_dim)) {
				auto &ntag = tagOf(nx, ny);
				int conductivity = std::max<int>(0, tag.heat - ntag.heat)
					* std::min(tag.thermal_conductivity,
				               ntag.thermal_conductivity);

				int received_heat = std::round(
					1.f * tag.heat * conductivity / total_thermal_conductivity
				);
				transfer_amount += received_heat;
				next_heat[ny * _width + nx] += received_heat;
			}
			next_heat[y * _width + x] += tag.heat - transfer_amount;
		}
	}

	// Heat decay
	const auto rng_max = std::decay<decltype(rng)>::type::max();
	for (int i = 0; i < _width * _height; ++i) {
		float delta = next_heat[i] * heat_decay_factor;
		int nat = std::floor(delta);
		float frac = delta - nat;
		next_heat[i] -= nat;
		if (rng() < std::round(frac * static_cast<double>(rng_max))) {
			next_heat[i] -= 1;
		}
	}

	for (int i = 0; i < _width * _height; ++i) {
		_tags[i].heat = std::clamp(next_heat[i], 0u, PixelTag::heat_max);
	}
}

} // namespace wf
