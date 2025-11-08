#include "wforge/2d.h"
#include "wforge/colorpalette.h"
#include "wforge/fallsand.h"
#include <cmath>
#include <cstddef>

namespace wf {
namespace element {

std::size_t Water::hash() const noexcept {
	const std::size_t magic = ('W' << 24) | ('A' << 16) | ('T' << 8) | 'R';
	return magic;
}

PixelTag Water::newTag() const noexcept {
	return PixelTag{
		.type = PixelType::Water,
		.pclass = PixelClass::Fluid,
		.color_index = colorIndexOf("Water"),
	};
}

void Water::step(PixelWorld &world, int x, int y) noexcept {
	if (y + 1 >= world.height()) {
		world.replacePixelWithAir(x, y);
		return;
	}

	auto &my_tag = world.tagOf(x, y);

	auto below_tag = world.tagOf(x, y + 1);
	if (below_tag.pclass == PixelClass::Gas) {
		if (my_tag.is_free_falling) {
			// Falling too fast, become particle
			world.replacePixel(
				x, y,
				pro::make_proxy<wf::PixelFacade, wf::element::WaterParticle>(
					0.0f, 1.5f
				)
			);
		} else {
			my_tag.is_free_falling = true;
		}
		world.swapPixels(x, y, x, y + 1);
		return;
	}
	my_tag.is_free_falling = false;

	if (my_tag.fluid_dir == 0) {
		my_tag.fluid_dir = (world.rand() % 2 == 0) ? 1 : -1;
	}

	bool tiny_water_flow = below_tag.pclass == PixelClass::Solid;
	for (auto d : std::array<int, 2>{my_tag.fluid_dir, -my_tag.fluid_dir}) {
		int new_x = x + d;
		if (new_x < 0 || new_x >= world.width()) {
			world.replacePixelWithAir(x, y);
			return;
		}

		auto diag_tag = world.tagOf(new_x, y + 1);
		if (diag_tag.pclass == PixelClass::Gas) {
			my_tag.fluid_dir = d;
			my_tag.is_free_falling = true;
			world.swapPixels(x, y, new_x, y + 1);
			break;
		}

		auto side_tag = world.tagOf(new_x, y);
		if (side_tag.pclass == PixelClass::Gas) {
			my_tag.fluid_dir = d;
			world.swapPixels(x, y, new_x, y);
			break;
		}

		if (tiny_water_flow && side_tag.pclass == PixelClass::Fluid
		    && side_tag.fluid_dir != 0) {
			my_tag.fluid_dir = side_tag.fluid_dir;
			return;
		}
	}
	my_tag.fluid_dir = 0;
}

WaterParticle::WaterParticle(float init_vx, float init_vy) noexcept
	: vx(init_vx), vy(init_vy) {}

std::size_t WaterParticle::hash() const noexcept {
	const std::size_t magic = ('W' << 24) | ('P' << 16) | ('A' << 8) | 'R';
	std::hash<float> float_hasher;
	return float_hasher(vx) ^ (float_hasher(vy) << 1) ^ magic;
}

PixelTag WaterParticle::newTag() const noexcept {
	return PixelTag{
		.type = PixelType::WaterParticle,
		.pclass = PixelClass::Particle,
		.color_index = colorIndexOf("Water"),
	};
}

constexpr float air_drag = 0.95f;
constexpr float bounce_back_y2x_factor = 0.2f;
constexpr float bounce_back_decay = 0.6f;

void WaterParticle::step(PixelWorld &world, int x, int y) noexcept {
	if (y + 1 >= world.height()) {
		world.replacePixelWithAir(x, y);
		return;
	}

	vy += PixelWorld::gAcceleration;
	vx *= air_drag;
	vy *= air_drag;

	int target_x = x + std::round(vx);
	int target_y = y + std::round(vy);
	bool forced_stop = false;

	int to_x = x, to_y = y;
	for (auto [tx, ty] : tilesOnSegment({x, y}, {target_x, target_y})) {
		if (tx == x && ty == y) {
			continue;
		}

		if (tx < 0 || tx >= world.width() || ty < 0 || ty >= world.height()) {
			world.replacePixelWithAir(x, y);
			return;
		}

		auto tag = world.tagOf(tx, ty);
		if (tag.pclass == PixelClass::Solid
		    || tag.pclass == PixelClass::Fluid) {
			forced_stop = true;
			break;
		}

		if (tag.pclass == PixelClass::Gas) {
			to_x = tx;
			to_y = ty;
		}
	}

	if (vy > 0 && to_y + 1 < world.height()) {
		forced_stop |= world.classOfIs(to_x, to_y + 1, PixelClass::Solid);
		forced_stop |= world.classOfIs(to_x, to_y + 1, PixelClass::Fluid);
	} else if (vy < 0 && to_y - 1 >= 0) {
		forced_stop |= world.classOfIs(to_x, to_y - 1, PixelClass::Solid);
		forced_stop |= world.classOfIs(to_x, to_y - 1, PixelClass::Fluid);
	}

	if (forced_stop) {
		bool free_dir[2] = {false, false};
		for (int d : {-1, 1}) {
			int side_x = to_x + d;
			if (side_x < 0 || side_x >= world.width()) {
				free_dir[(d + 1) / 2] = true;
				continue;
			}

			auto side_tag = world.tagOf(side_x, to_y);
			free_dir[(d + 1) / 2]
				= (side_tag.pclass != PixelClass::Solid
			       && side_tag.pclass != PixelClass::Fluid);
		}

		vx *= bounce_back_decay;
		if (std::abs(vx) < 0.01f) {
			int rand_dir = (world.rand() % 2) * 2 - 1; // -1 or +1
			for (int d : {rand_dir, -rand_dir}) {
				if (free_dir[(d + 1) / 2]) {
					vx = d * std::abs(vy) * bounce_back_y2x_factor;
					break;
				}
			}
		} else if (vx < 0) {
			vx -= vy * bounce_back_y2x_factor;
			if (!free_dir[0]) {
				if (free_dir[1]) {
					vx = -vx * bounce_back_decay;
				} else {
					vx = 0;
				}
			}
		} else if (vx > 0) {
			vx += vy * bounce_back_y2x_factor;
			if (!free_dir[1]) {
				if (free_dir[0]) {
					vx = -vx * bounce_back_decay;
				} else {
					vx = 0;
				}
			}
		}

		vy *= -bounce_back_y2x_factor;
	}

	if (to_x != x || to_y != y) {
		world.swapPixels(x, y, to_x, to_y);
		return;
	}

	if (vx * vx + vy * vy < 2) {
		int dir = 0;
		if (std::abs(vx) < 0.01) {
			dir = 0;
		} else if (vx < 0) {
			dir = -1;
		} else {
			dir = 1;
		}
		world.replacePixel(
			x, y, pro::make_proxy<wf::PixelFacade, wf::element::Water>()
		);
		world.tagOf(x, y).fluid_dir = dir;
	}
}

} // namespace element
} // namespace wf