#include "wforge/2d.h"
#include "wforge/fallsand.h"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <random>
#include <utility>
#include <vector>

namespace wf {

namespace {

using Coord = std::array<int, 2>;

struct ConnectedComponent {
	int id;
	std::vector<Coord> surface_pixels;
};

struct AnalysisContext {
	std::vector<ConnectedComponent> components;
	std::vector<Coord> fluid_pixels;
};

bool isAirLike(const PixelTag &tag) noexcept {
	return tag.pclass == PixelClass::Gas || tag.pclass == PixelClass::Particle;
}

constexpr float surface_adjust_factor = 0.7;

void densityAnalysisStep(PixelWorld &world, AnalysisContext &ctx) noexcept {
	const int efftective_infinity_of_x = world.width() + 10;

	for (int y = world.height() - 2; y >= 0; --y) {
		std::vector<std::pair<int, int>> active_ranges; // [l, r]

		for (int l = 0, r = 0; r < world.width(); ++r) {
			auto tag = world.tagOf(r, y);

			if (tag.pclass == PixelClass::Fluid && r + 1 < world.width()) {
				continue;
			}

			if (r > l) {
				active_ranges.emplace_back(
					l, tag.pclass == PixelClass::Fluid ? r : r - 1
				);
			}
			l = r + 1;
		}

		for (auto [l, r] : active_ranges) {
			if (l == r) {
				continue;
			}

			std::vector<int> fill_pos;
			PixelType fill_type = PixelType::Air;
			bool at_least_dual_fluids = false;
			for (int x = l; x <= r; ++x) {
				auto tag = world.tagOf(x, y + 1);
				if (tag.pclass != PixelClass::Fluid) {
					continue;
				}

				if (fill_type != PixelType::Air && fill_type != tag.type) {
					at_least_dual_fluids = true;
				}

				if (fill_type == PixelType::Air
				    || isDenser(fill_type, tag.type)) {
					fill_type = tag.type;
					fill_pos.clear();
				}

				if (tag.type == fill_type) {
					fill_pos.push_back(x);
				}
			}

			int avail_count = fill_pos.size();
			if (avail_count == 0 || !at_least_dual_fluids) {
				continue;
			}

			std::vector<int> left_pos;
			int sp = 0;
			for (int x = l; x <= r; ++x) {
				while (sp < fill_pos.size()
				       && (fill_pos[sp] == -1 || fill_pos[sp] < x)) {
					sp += 1;
				}

				if (sp < fill_pos.size() && fill_pos[sp] == x) {
					left_pos.push_back(x);
				}

				if (world.typeOfIs(x, y, fill_type)) {
					continue;
				}

				bool has_option = false;
				int left_dis = efftective_infinity_of_x;
				int right_dis = efftective_infinity_of_x;
				int lp, rp;
				if (!left_pos.empty()) {
					has_option = true;
					lp = left_pos.back();
					left_dis = x - lp;
				}

				if (sp < fill_pos.size()) {
					has_option = true;
					rp = fill_pos[sp];
					right_dis = rp - x;
				}

				if (!has_option) {
					continue;
				}

				avail_count -= 1;
				if (left_dis < right_dis) {
					world.swapFluids(x, y, lp, y + 1);
					left_pos.pop_back();
				} else {
					world.swapFluids(x, y, rp, y + 1);
					fill_pos[sp] = -1;
				}

				if (avail_count == 0) {
					break;
				}
			}
		}
	}
}

} // namespace

void PixelWorld::fluidAnalysisStep() noexcept {
	AnalysisContext ctx;

	std::memset(_fluid_cid.get(), -1, sizeof(int) * _width * _height);

	// Step 1: Identify fluid connected components
	Coord world_dim = {_width, _height};
	for (int x = 0; x < _width; ++x) {
		for (int y = 0; y < _height; ++y) {
			if (!classOfIs(x, y, PixelClass::Fluid)) {
				continue;
			}

			if (tagOf(x, y).dirty) {
				continue;
			}

			// New component found
			ConnectedComponent comp = {
				.id = static_cast<int>(ctx.components.size()),
			};

			// Flood fill
			std::vector<Coord> stack;
			stack.push_back({x, y});
			while (!stack.empty()) {
				auto [cx, cy] = stack.back();
				stack.pop_back();

				tagOf(cx, cy).dirty = true;
				_fluid_cid[cy * _width + cx] = comp.id;
				ctx.fluid_pixels.push_back({cx, cy});

				// Check neighbors
				for (auto [nx, ny] : neighborsOf({cx, cy}, world_dim)) {
					if (!classOfIs(nx, ny, PixelClass::Fluid)) {
						continue;
					}
					if (tagOf(nx, ny).dirty) {
						continue;
					}
					stack.push_back({nx, ny});
				}
			}
			ctx.components.push_back(std::move(comp));
		}
	}
	resetDirtyFlags();

	// Step 2: Analyze horizontally flowing fluids
	for (int y = 0; y < _height; ++y) {
		int last_comp_id = -1;
		int dircnt[3] = {0, 0, 0}; // -1, 0, +1
		std::vector<int> loc;

		auto handle = [&]() {
			if (last_comp_id == -1) {
				return;
			}

			for (int p : loc) {
				auto &tag = tagOf(p, y);
				if (dircnt[0] > 0) {
					tag.fluid_dir = -1;
					dircnt[0] -= 1;
				} else if (dircnt[1] > 0) {
					tag.fluid_dir = 0;
					dircnt[1] -= 1;
				} else {
					assert(dircnt[2] > 0);
					tag.fluid_dir = 1;
				}
			}

			loc.clear();
			last_comp_id = -1;
			dircnt[0] = dircnt[1] = dircnt[2] = 0;
		};

		for (int x = 0; x < _width; ++x) {
			auto tag = tagOf(x, y);
			if (tag.pclass == PixelClass::Solid) {
				handle();
				continue;
			}

			if (tag.pclass != PixelClass::Fluid) {
				continue;
			}
			dircnt[tag.fluid_dir + 1] += 1;
			loc.push_back(x);

			auto cid = _fluid_cid[y * _width + x];
			if (last_comp_id != cid) {
				handle();
				last_comp_id = cid;
			}
		}
		handle();
	}

	// Step 3: Analyze surface pixels
	for (auto coord : ctx.fluid_pixels) {
		auto [x, y] = coord;
		auto &comp = ctx.components[_fluid_cid[y * _width + x]];
		if (y == 0
		    || isAirLike(tagOf(x, y - 1)) && !tagOf(x, y).is_free_falling) {
			comp.surface_pixels.push_back({x, y});
		}
	}

	for (auto &comp : ctx.components) {
		int n = comp.surface_pixels.size();
		if (n < 2) {
			continue;
		}

		// Sort by y ascending, then x ascending
		std::sort(
			comp.surface_pixels.begin(), comp.surface_pixels.end(),
			[](const Coord &a, const Coord &b) {
			if (a[1] != b[1]) {
				return a[1] < b[1];
			}
			return a[0] < b[0];
		}
		);

		std::mt19937 rng(rand());
		// shuffle pixels with the same y to avoid artifacts
		auto l = comp.surface_pixels.begin();
		for (auto r = comp.surface_pixels.begin();
		     r != comp.surface_pixels.end(); ++r) {
			if (r->at(1) != l->at(1)) {
				std::shuffle(l, r, rng);
				l = r;
			}
		}
		if (l != comp.surface_pixels.end()) {
			std::shuffle(l, comp.surface_pixels.end(), rng);
		}

		int p = 0;
		int cur_remove_y = comp.surface_pixels[0][1];
		while (p < n && comp.surface_pixels[p][1] == cur_remove_y) {
			p += 1;
		}

		int max_delta_y = comp.surface_pixels.back()[1] - cur_remove_y;
		p = std::min<int>(p, std::round(max_delta_y * surface_adjust_factor));

		for (int i = 0; i <= p && p + i + 1 < n; ++i) {
			auto [ax, ay] = comp.surface_pixels[i];
			auto [bx, by] = comp.surface_pixels[n - 1 - i];
			if (by - 1 <= ay) {
				break;
			}
			swapPixels(ax, ay, bx, by - 1);
		}
	}

	densityAnalysisStep(*this, ctx);
}

} // namespace wf