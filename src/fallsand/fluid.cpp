#include "wforge/2d.h"
#include "wforge/fallsand.h"
#include <algorithm>
#include <cassert>
#include <map>
#include <vector>

namespace wf {

namespace {

using Coord = std::array<int, 2>;

struct ConnectedComponent {
	int id;
	int size = 0;

	// Remark: heighest_y < lowest_y (y increases downwards)
	int heighest_y = 0;
	int lowest_y = 0;

	std::vector<Coord> surface_pixels;
};

struct AnalysisContext {
	std::vector<ConnectedComponent> components;

	// Why not a plain array here?
	// It mostly depends on the ratio of fluid pixels to total pixels.
	// Let's assume the majority of pixels are non-fluid, or even basically air.
	// Why not std::unordered_map?
	// Although std::map is O(log n), in partical use it's usually faster to
	// std::unordered_map due to better cache locality and other factors.
	std::map<Coord, int> pixel_to_component;
};

bool isAirLike(const PixelTag &tag) noexcept {
	return tag.pclass == PixelClass::Gas || tag.pclass == PixelClass::Particle;
}

} // namespace

void PixelWorld::fluidAnalysisStep() noexcept {
	AnalysisContext ctx;

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
				.size = 0,
				.heighest_y = y,
				.lowest_y = y,
			};

			// Flood fill
			std::vector<Coord> stack;
			stack.push_back({x, y});
			while (!stack.empty()) {
				auto [cx, cy] = stack.back();
				stack.pop_back();

				tagOf(cx, cy).dirty = true;
				ctx.pixel_to_component[{cx, cy}] = comp.id;

				comp.size += 1;
				comp.heighest_y = std::min(comp.heighest_y, cy);
				comp.lowest_y = std::max(comp.lowest_y, cy);

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

			auto cid = ctx.pixel_to_component[{x, y}];
			if (last_comp_id != cid) {
				handle();
				last_comp_id = cid;
			}
		}
		handle();
	}

	// Step 3: Analyze surface pixels
	for (const auto &[coord, comp_id] : ctx.pixel_to_component) {
		auto [x, y] = coord;
		auto &comp = ctx.components[comp_id];
		if (y == 0 || isAirLike(tagOf(x, y - 1))) {
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

		int p = 0;
		int cur_remove_y = comp.surface_pixels[0][1];
		while (p < n && comp.surface_pixels[p][1] == cur_remove_y) {
			p += 1;
		}

		for (int i = 0; i <= p && p + i + 1 < n; ++i) {
			auto [ax, ay] = comp.surface_pixels[i];
			auto [bx, by] = comp.surface_pixels[n - 1 - i];
			swapPixels(ax, ay, bx, by - 1);
		}
	}
}

} // namespace wf