#include "wforge/level.h"
#include "wforge/assets.h"
#include <algorithm>
#include <cmath>
#include <cpptrace/cpptrace.hpp>
#include <vector>

namespace wf {

namespace {

constexpr float duck_buoyancy_factor = .02f;
constexpr float duck_flow_factor = .05f;
constexpr float duck_air_drag = 0.95f;
constexpr float duck_fluid_drag = 0.8f;
constexpr float duck_ground_friction = 0.7f;

struct RelatedPixel {
	int x;
	int y;
	float area;

	RelatedPixel(int x, int y, float area) noexcept: x(x), y(y), area(area) {}
};

} // namespace

DuckEntity::DuckEntity(sf::Vector2f pos) noexcept
	: shape(*AssetsManager::instance().getAsset<PixelShape>("duck/shape"))
	, position(pos)
	, velocity(0.0f, 0.0f) {}

void DuckEntity::step(const Level &level) noexcept {
	const auto &world = level.fallsand;

	// Apply gravity
	velocity.y += PixelWorld::gAcceleration;

	// Calculate related pixels for buoyancy
	std::vector<RelatedPixel> raw_related_pixels;
	for (int dx = 0; dx < shape.width(); ++dx) {
		for (int dy = 0; dy < shape.height(); ++dy) {
			if (!shape.hasPixel(dx, dy)) {
				continue;
			}

			int fx = std::floor(position.x) + dx;
			int cx = std::ceil(position.x) + dx;
			int fy = std::floor(position.y) + dy;
			int cy = std::ceil(position.y) + dy;

			for (int px : {fx, cx}) {
				for (int py : {fy, cy}) {
					if (px < 0 || px >= world.width() || py < 0
					    || py >= world.height()) {
						continue;
					}

					// calculate ratio (related pixel area)
					float rx = position.x + dx - px;
					float ry = position.y + dy - py;
					float area = (1.0f - std::abs(rx)) * (1.0f - std::abs(ry));
					raw_related_pixels.emplace_back(px, py, area);
				}
			}
		}
	}

	// Merge related pixels entries
	std::vector<RelatedPixel> related_pixels;
	std::sort(
		raw_related_pixels.begin(), raw_related_pixels.end(),
		[](const RelatedPixel &a, const RelatedPixel &b) {
		if (a.x != b.x) {
			return a.x < b.x;
		}
		return a.y < b.y;
	}
	);

	for (std::size_t i = 0; i < raw_related_pixels.size();) {
		int cx = raw_related_pixels[i].x;
		int cy = raw_related_pixels[i].y;
		float total_area = 0.0f;

		std::size_t j;
		for (j = i; j < raw_related_pixels.size()
		     && raw_related_pixels[j].x == cx && raw_related_pixels[j].y == cy;
		     ++j) {
			total_area += raw_related_pixels[j].area;
		}

		related_pixels.emplace_back(cx, cy, total_area);
		i = j;
	}

	// Apply buoyancy
	float in_water_area = .0;
	for (const auto &rp : related_pixels) {
		if (world.classOfIs(rp.x, rp.y, PixelClass::Fluid)) {
			in_water_area += rp.area;
		}
	}
	velocity.y -= duck_buoyancy_factor * in_water_area;

	// Apply water flow
	float total_flow = .0;
	for (const auto &rp : related_pixels) {
		if (world.classOfIs(rp.x, rp.y, PixelClass::Fluid)) {
			const auto &tag = world.tagOf(rp.x, rp.y);
			total_flow += tag.fluid_dir * rp.area;
		}
	}
	velocity.x += duck_flow_factor * total_flow;

	// Apply drag
	float total_drag = .0f;
	float drag_involved = .0f;
	for (const auto &rp : related_pixels) {
		if (world.classOfIs(rp.x, rp.y, PixelClass::Fluid)) {
			total_drag += duck_fluid_drag * rp.area;
			drag_involved += rp.area;
		} else if (world.classOfIs(rp.x, rp.y, PixelClass::Gas)) {
			total_drag += duck_air_drag * rp.area;
			drag_involved += rp.area;
		}
	}
	if (drag_involved > 0.0f) {
		float avg_drag = total_drag / drag_involved;
		velocity.x *= avg_drag;
		velocity.y *= avg_drag;
	}

	// Apply friction when on ground
	bool on_ground = false;
	int foot_y = std::round(position.y + shape.height());
	for (int dx = 0; dx < shape.width(); ++dx) {
		if (!shape.hasPixel(dx, shape.height() - 1)) {
			continue;
		}

		int foot_x = std::round(position.x) + dx;
		if (foot_x < 0 || foot_x >= world.width() || foot_y < 0
		    || foot_y >= world.height()) {
			continue;
		}

		if (world.classOfIs(foot_x, foot_y, PixelClass::Solid)) {
			on_ground = true;
			break;
		}
	}
	if (on_ground) {
		velocity.x *= duck_ground_friction;
	}

	// Update position
	int cur_x = std::round(position.x);
	int cur_y = std::round(position.y);
	int target_x = std::round(position.x + velocity.x);
	int target_y = std::round(position.y + velocity.y);

	int to_x = cur_x, to_y = cur_y;

	bool collision = false;
	for (auto [tx, ty] : tilesOnSegment({cur_x, cur_y}, {target_x, target_y})) {
		// check collision
		for (int dx = 0; dx < shape.width(); ++dx) {
			for (int dy = 0; dy < shape.height(); ++dy) {
				if (!shape.hasPixel(dx, dy)) {
					continue;
				}

				int wx = tx + dx;
				int wy = ty + dy;
				if (wx < 0 || wx >= world.width() || wy < 0
				    || wy >= world.height()) {
					collision = true;
					break;
				}

				if (world.classOfIs(wx, wy, PixelClass::Solid)) {
					collision = true;
					break;
				}
			}

			if (collision) {
				break;
			}
		}

		if (collision) {
			// stop movement upon collision
			velocity.x = 0.0f;
			velocity.y = 0.0f;
			break;
		}

		to_x = tx;
		to_y = ty;
	}

	if (collision) {
		position.x = to_x;
		position.y = to_y;
	} else {
		position += velocity;
	}
}

Level::Level(int width, int height) noexcept: fallsand(width, height) {}

void Level::step() {
	fallsand.step();
	duck.step(*this);
}

} // namespace wf
