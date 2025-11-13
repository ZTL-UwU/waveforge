#include "wforge/2d.h"
#include "wforge/colorpalette.h"
#include "wforge/fallsand.h"
#include "wforge/xoroshiro.h"
#include <cmath>
#include <random>

namespace wf {
namespace element {

PixelTag Sand::newTag() const noexcept {
	return PixelTag{
		.type = PixelType::Sand,
		.pclass = PixelClass::Solid,
		.color_index = colorIndexOf("Sand"),
		.is_free_falling = true,
		.thermal_conductivity = 2,
	};
}

namespace {

bool isSwappaleTag(PixelTag tag) noexcept {
	return tag.type == PixelType::Air || tag.pclass == PixelClass::Fluid
		|| tag.pclass == PixelClass::Particle;
}

constexpr float sandFriction = 0.8f;
constexpr float airDrag = 0.95f;
constexpr float waterDrag = 0.8f;
constexpr float bounceBackYFactor = 0.3f;
constexpr float bounceBackXFactor = 0.4f;
constexpr int inertialResistance = 10;
constexpr float sand_mass = 3.0f;

} // namespace

void Sand::step(PixelWorld &world, int x, int y) noexcept {
	if (y + 1 >= world.height()) {
		// at bottom edge, remove sand
		world.replacePixelWithAir(x, y);
		return;
	}

	auto &my_tag = world.tagOf(x, y);

	vy += PixelWorld::gAcceleration;

	auto below_tag = world.tagOf(x, y + 1);
	if (below_tag.pclass == PixelClass::Solid) {
		// Is on ground. Apply friction
		vx *= sandFriction;
		vy = std::min(0.0f, vy);
	} else if (below_tag.pclass == PixelClass::Fluid) {
		vx *= waterDrag;
		vy *= waterDrag;
		my_tag.is_free_falling = true;
	} else {
		vx *= airDrag;
		vy *= airDrag;
		my_tag.is_free_falling = true;
	}

	auto rng = Xoroshiro128PP::globalInstance();

	int target_x = x + std::round(vx);
	int target_y = y + std::round(vy);
	if (target_x == x && target_y == y && my_tag.is_free_falling) {
		// Consider diagonal swap
		if (below_tag.pclass != PixelClass::Solid) {
			return;
		}
		int rand_dir = (rng.next() % 2 == 0) ? -1 : 1;
		for (int d : {rand_dir, -rand_dir}) {
			int new_x = x + d;
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
		my_tag.is_free_falling = false;
		return;
	}

	auto to_x = x, to_y = y;
	bool forced_stop = false;
	const std::array<int, 2> world_dim = {world.width(), world.height()};
	std::uniform_int_distribution<int> inertial_dist(0, inertialResistance - 1);
	for (auto [tx, ty] : tilesOnSegment({x, y}, {target_x, target_y})) {
		if (tx < 0 || tx >= world.width() || ty < 0 || ty >= world.height()) {
			// Out of bounds
			world.replacePixelWithAir(x, y);
			return;
		}

		auto target_tag = world.tagOf(tx, ty);
		if ((tx != x || ty != y) && !isSwappaleTag(target_tag)) {
			// Can't move further? Stop here
			forced_stop = true;
			break;
		}

		to_x = tx;
		to_y = ty;
		if (my_tag.is_free_falling) {
			for (auto [nx, ny] : neighborsOf({tx, ty}, world_dim)) {
				if (inertial_dist(rng) == 0) {
					continue;
				}
				world.tagOf(nx, ny).is_free_falling = true;
			}
		}
	}

	if (vy > 0 && to_y + 1 < world.height()) {
		auto below_tag_after = world.tagOf(to_x, to_y + 1);
		if (below_tag_after.pclass == PixelClass::Solid
		    && !below_tag_after.is_free_falling) {
			forced_stop = true;
		}
	} else if (vy < 0 && to_y - 1 >= 0) {
		auto above_tag = world.tagOf(to_x, to_y - 1);
		if (above_tag.pclass == PixelClass::Solid) {
			forced_stop = true;
		}
	}

	if (forced_stop) {
		bool freeDir[2] = {false, false};
		for (int d : {-1, 1}) {
			int side_x = to_x + d;
			if (side_x < 0 || side_x >= world.width()) {
				freeDir[(d + 1) / 2] = true;
				continue;
			}

			auto side_tag = world.tagOf(side_x, to_y);
			freeDir[(d + 1) / 2] = (side_tag.pclass != PixelClass::Solid);
		}

		if (std::abs(vx) < 0.01f) {
			int rand_dir = (rng.next() % 2 == 0) ? -1 : 1;
			for (int d : {rand_dir, -rand_dir}) {
				if (freeDir[(d + 1) / 2]) {
					vx = d * std::abs(vy) * bounceBackXFactor;
					break;
				}
			}
		} else if (vx < 0) {
			vx -= vy * bounceBackXFactor;
			if (!freeDir[0]) {
				if (freeDir[1]) {
					vx = -vx * bounceBackXFactor;
				} else {
					vx = 0;
				}
			}
		} else if (vx > 0) {
			vx += vy * bounceBackXFactor;
			if (!freeDir[1]) {
				if (freeDir[0]) {
					vx = -vx * bounceBackXFactor;
				} else {
					vx = 0;
				}
			}
		}
		vy *= -bounceBackYFactor;
	}

	if (to_x != x || to_y != y) {
		world.swapPixels(x, y, to_x, to_y);
	}
}

} // namespace element
} // namespace wf