#ifndef WFORGE_FALLSAND_H
#define WFORGE_FALLSAND_H

#include "wforge/2d.h"
#include <cstdint>
#include <memory>
#include <proxy/proxy.h>
#include <proxy/v4/proxy.h>
#include <proxy/v4/proxy_macros.h>

namespace wf {

namespace _dispatch {

PRO_DEF_MEM_DISPATCH(MemHash, hash);
PRO_DEF_MEM_DISPATCH(MemNewTag, newTag);
PRO_DEF_MEM_DISPATCH(MemStep, step);

} // namespace _dispatch

struct PixelTag;
class PixelWorld;

/* clang-format off */
struct PixelFacade : pro::facade_builder
	::add_convention<_dispatch::MemHash, std::size_t() const noexcept>
	::add_convention<_dispatch::MemNewTag, PixelTag() const noexcept>
	::add_convention<_dispatch::MemStep, void(PixelWorld &world, int x, int y) noexcept>
	::build {};
/* clang-format on */

using PixelElement = pro::proxy<PixelFacade>;

enum class PixelType : std::uint8_t {
	Air,
	Stone,
	Sand,
	Water,
	WaterParticle,
};

enum class PixelClass : std::uint8_t {
	Solid = 0,
	Fluid,
	Gas,
	Particle,
};

struct PixelTag {
	PixelType type : 6;
	PixelClass pclass : 2;
	unsigned int color_index : 8; // 256 colors
	bool dirty : 1 = false;
	bool is_free_falling : 1 = false;
	signed int fluid_dir : 2; // -1 = left, 0 = none, +1 = right
};

class PixelWorld {
public:
	constexpr static float gAcceleration = 0.5f;

	PixelWorld();
	PixelWorld(int width, int height);

	int width() const noexcept {
		return _width;
	}

	int height() const noexcept {
		return _height;
	}

	unsigned int rand() noexcept;

	PixelTag tagOf(int x, int y) const noexcept;
	PixelTag &tagOf(int x, int y) noexcept;
	PixelElement &elementOf(int x, int y) noexcept;

	void swapPixels(int x1, int y1, int x2, int y2) noexcept;
	void replacePixel(int x, int y, PixelElement new_pixel) noexcept;
	void replacePixelWithAir(int x, int y) noexcept;

	bool typeOfIs(int x, int y, PixelType ptype) const noexcept;
	bool classOfIs(int x, int y, PixelClass pclass) const noexcept;

	void step() noexcept;

protected:
	void resetDirtyFlags() noexcept;
	void fluidAnalysisStep() noexcept;

private:
	int _width;
	int _height;

	std::unique_ptr<PixelTag[]> _tags;
	std::unique_ptr<PixelElement[]> _elements;
};

namespace element {

struct EmptySubsElement {
	void step(PixelWorld &world, int x, int y) noexcept {}
};

struct SolidElement : EmptySubsElement {
	void dealtPressure(
		PixelWorld &world, int x, int y, float pressure, Direction from_dir
	) noexcept;
};

struct Air : EmptySubsElement {
	std::size_t hash() const noexcept;
	PixelTag newTag() const noexcept;
};

struct Stone : SolidElement {
	std::size_t hash() const noexcept;
	PixelTag newTag() const noexcept;
};

struct Sand : SolidElement {
	std::size_t hash() const noexcept;
	PixelTag newTag() const noexcept;
	void step(PixelWorld &world, int x, int y) noexcept;

protected:
	float vx = 0, vy = 0;
};

struct Water : EmptySubsElement {
	std::size_t hash() const noexcept;
	PixelTag newTag() const noexcept;
	void step(PixelWorld &world, int x, int y) noexcept;
};

struct WaterParticle : EmptySubsElement {
	std::size_t hash() const noexcept;
	PixelTag newTag() const noexcept;
	void step(PixelWorld &world, int x, int y) noexcept;

	WaterParticle() noexcept = default;
	WaterParticle(float init_vx, float init_vy) noexcept;

protected:
	float vx = 0, vy = 0;
};

} // namespace element

} // namespace wf

#endif // WFORGE_FALLSAND_H
