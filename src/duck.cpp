#include "wforge/2d.h"
#include "wforge/assets.h"
#include "wforge/fallsand.h"
#include "wforge/level.h"
#include "wforge/xoroshiro.h"
#include <SFML/Window/Joystick.hpp>
#include <cmath>

namespace wf {

namespace {

constexpr float duck_buoyancy_factor = .02f;
constexpr float duck_flow_factor = .03f;
constexpr float duck_steam_jet_factor = 0.07f;
constexpr float duck_air_drag = 0.95f;
constexpr float duck_fluid_drag = 0.7f;
constexpr float duck_ground_friction = 0.7f;
constexpr float duck_solid_correction_factor = 0.01f;
constexpr float duck_solid_correction_threshold = 1.5f;

struct RelatedPixel {
	int x;
	int y;
	float area;

	RelatedPixel(int x, int y, float area) noexcept: x(x), y(y), area(area) {}
};

} // namespace

DuckEntity::DuckEntity(sf::Vector2f pos) noexcept
	: shape(AssetsManager::instance().getAsset<PixelShape>("duck/shape"))
	, position(pos)
	, velocity(0.0f, 0.0f) {}

void DuckEntity::setPosition(float x, float y) noexcept {
	position.x = x;
	position.y = y;
}

bool DuckEntity::isOutOfWorld(const Level &level) const noexcept {
	auto width = level.width();
	auto height = level.height();
	constexpr int padding = 10;
	if (position.x + shape.width() < -padding) {
		return true;
	}

	if (position.x > width + padding) {
		return true;
	}

	if (position.y + shape.height() < -padding) {
		return true;
	}

	if (position.y > height + padding) {
		return true;
	}
	return false;
}

bool DuckEntity::willCollideAt(
	const Level &level, int target_x, int target_y
) const noexcept {
	auto &world = level.fallsand;
	for (int dx = 0; dx < shape.width(); ++dx) {
		int wx = target_x + dx;
		if (wx < 0 || wx >= world.width()) {
			continue;
		}

		for (int dy = 0; dy < shape.height(); ++dy) {
			if (!shape.hasPixel(dx, dy)) {
				continue;
			}

			int wy = target_y + dy;
			if (wy < 0 || wy >= world.height()) {
				continue;
			}

			auto tag = world.tagOf(wx, wy);
			if (tag.pclass == PixelClass::Solid && !tag.is_free_falling) {
				return true;
			}
		}
	}
	return false;
}

bool DuckEntity::currentlyColliding(const Level &level) const noexcept {
	int cur_x = std::round(position.x);
	int cur_y = std::round(position.y);
	return willCollideAt(level, cur_x, cur_y);
}

void DuckEntity::step(const Level &level) noexcept {
	const auto &world = level.fallsand;

	// Apply gravity
	velocity.y += PixelWorld::gAcceleration;

	// Calculate related pixels for buoyancy
	std::vector<RelatedPixel> raw_related_pixels;
	int floor_x = std::floor(position.x);
	int ceil_x = std::ceil(position.x);
	int floor_y = std::floor(position.y);
	int ceil_y = std::ceil(position.y);
	for (int dx = 0; dx < shape.width(); ++dx) {
		for (int dy = 0; dy < shape.height(); ++dy) {
			if (!shape.hasPixel(dx, dy)) {
				continue;
			}

			int fx = floor_x + dx;
			int cx = ceil_x + dx;
			int fy = floor_y + dy;
			int cy = ceil_y + dy;

			for (int px : {fx, cx}) {
				for (int py : {fy, cy}) {
					if (!world.inBounds(px, py)) {
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

	// Apply steam jet
	float total_steam_force = .0f;
	for (const auto &rp : related_pixels) {
		auto tag = world.tagOf(rp.x, rp.y);
		if (tag.type == PixelType::Steam) {
			total_steam_force += rp.area * duck_steam_jet_factor;
		}
	}
	velocity.y -= total_steam_force;

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

	// Apply solid collision correction force
	float in_solid_area = .0f;
	for (const auto &rp : related_pixels) {
		auto tag = world.tagOf(rp.x, rp.y);
		if (tag.pclass == PixelClass::Solid && !tag.is_free_falling) {
			in_solid_area += rp.area;
		}
	}
	if (in_solid_area > 0.01f) {
		velocity.y -= std::max(
			duck_solid_correction_factor * in_solid_area,
			PixelWorld::gAcceleration + 0.1f
		);

		if (velocity.y < -duck_solid_correction_threshold) {
			velocity.y = -duck_solid_correction_threshold;
		}
	}

	// Update position
	int cur_x = std::round(position.x);
	int cur_y = std::round(position.y);
	int target_x = std::round(position.x + velocity.x);
	int target_y = std::round(position.y + velocity.y);

	int to_x = cur_x, to_y = cur_y;

	bool cur_colliding = currentlyColliding(level);
	bool collision_allowed = cur_colliding;
	bool forced_stop = false;
	for (auto [tx, ty] : tilesOnSegment({cur_x, cur_y}, {target_x, target_y})) {
		if (tx == cur_x && ty == cur_y) {
			continue;
		}

		bool collision = willCollideAt(level, tx, ty);
		if (collision && !collision_allowed) {
			forced_stop = true;
			break;
		}

		if (!collision) {
			collision_allowed = false;
		}

		to_x = tx;
		to_y = ty;
	}

	if (!forced_stop) {
		int target_fed_x = velocity.x > 0
			? std::ceil(position.x + velocity.x)
			: std::floor(position.x + velocity.x);
		int target_fed_y = velocity.y > 0
			? std::ceil(position.y + velocity.y)
			: std::floor(position.y + velocity.y);

		if (!willCollideAt(level, target_fed_x, target_fed_y)) {
			position += velocity;
		} else {
			position.x = target_x;
			position.y = target_y;
		}
		return;
	}

	if (std::abs(velocity.x) < 0.01f && target_y < to_y && !cur_colliding) {
		int rand_dir = (Xoroshiro128PP::globalInstance().next() % 2 == 0)
			? -1
			: 1;
		for (int d : {rand_dir, -rand_dir}) {
			int side_x = to_x + d;
			if (!willCollideAt(level, side_x, to_y - 2)) {
				to_x = side_x;
				to_y = to_y - 1;
			}
		}
	}

	position.x = to_x;
	position.y = to_y;

	velocity.x = 0.0f;
	velocity.y = 0.0f;
}

void DuckEntity::commitEntityPresence(PixelWorld &world) noexcept {
	int floor_x = std::floor(position.x);
	int ceil_x = std::ceil(position.x);
	int floor_y = std::floor(position.y);
	int ceil_y = std::ceil(position.y);

	for (int dx = 0; dx < shape.width(); ++dx) {
		for (int dy = 0; dy < shape.height(); ++dy) {
			if (!shape.hasPixel(dx, dy)) {
				continue;
			}

			int fx = floor_x + dx;
			int cx = ceil_x + dx;
			int fy = floor_y + dy;
			int cy = ceil_y + dy;

			for (int px : {fx, cx}) {
				for (int py : {fy, cy}) {
					if (!world.inBounds(px, py)) {
						continue;
					}

					auto &stag = world.staticTagOf(px, py);
					stag.external_entity_present = true;
				}
			}
		}
	}
}

} // namespace wf
