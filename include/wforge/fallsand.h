#ifndef WFORGE_FALLSAND_H
#define WFORGE_FALLSAND_H

#include <cstdint>
#include <memory>
#include <proxy/proxy.h>
#include <proxy/v4/proxy.h>
#include <proxy/v4/proxy_macros.h>
#include <span>

namespace wf {

namespace _dispatch {

// See microsoft/proxy library for the semantics of dispatch conventions
PRO_DEF_MEM_DISPATCH(MemNewTag, newTag);
PRO_DEF_MEM_DISPATCH(MemStep, step);

} // namespace _dispatch

struct PixelTag;
class PixelWorld;

/* clang-format off */
// See microsoft/proxy library for the semantics of proxy and facade
struct PixelFacade : pro::facade_builder
	::add_convention<_dispatch::MemNewTag, PixelTag() const noexcept>
	::add_convention<_dispatch::MemStep, void(PixelWorld &world, int x, int y) noexcept>
	::build {};
/* clang-format on */

using PixelElement = pro::proxy<PixelFacade>;

// Insert new types to correct position! Don't just append at the end!
enum class PixelType : std::uint8_t {
	// Gas types (order: lowest to highest density)
	Smoke,
	Steam,
	Air,

	// Particle types
	FluidParticle,

	// Fluid types (order: lowest to highest density)
	Oil,
	Water,

	// Solid types
	Stone,
	Wood,
	Copper,
	Sand,

	// for internal use only, keep at the end
	_count
};

static_assert(static_cast<std::uint8_t>(PixelType::_count) <= 64);

bool isDenser(PixelType a, PixelType b) noexcept;
bool isDenserOrEqual(PixelType a, PixelType b) noexcept;

enum class PixelClass : std::uint8_t {
	Solid = 0,
	Fluid,
	Gas,
	Particle,
};

struct PixelTag {
	static constexpr unsigned int heat_max = 127;
	static constexpr unsigned int thermal_conductivity_max = 63;

	PixelType type : 6;
	PixelClass pclass : 2;
	unsigned int color_index : 8; // 256 colors, see Colorpalette.h
	bool dirty : 1 = false;       // updated in current step, for physics
	bool is_free_falling : 1 = false;
	signed int fluid_dir : 2; // -1 = left, 0 = none, +1 = right
	unsigned int heat : 7 = 0;
	bool ignited : 1 = false; // on fire
	unsigned int thermal_conductivity : 6 = 0;
};

class PixelWorld {
public:
	constexpr static float gAcceleration = 0.5f;

	PixelWorld() noexcept;
	PixelWorld(int width, int height) noexcept;

	int width() const noexcept {
		return _width;
	}

	int height() const noexcept {
		return _height;
	}

	PixelTag tagOf(int x, int y) const noexcept;
	PixelTag &tagOf(int x, int y) noexcept;
	PixelElement &elementOf(int x, int y) noexcept;

	void swapPixels(int x1, int y1, int x2, int y2) noexcept;

	// swapPixels without swapping fluid_dir
	void swapFluids(int x1, int y1, int x2, int y2) noexcept;

	void replacePixel(int x, int y, PixelElement new_pixel) noexcept;
	void replacePixel(
		int x, int y, PixelElement new_pixel, PixelTag new_tag
	) noexcept;
	void replacePixelWithAir(int x, int y) noexcept;

	bool typeOfIs(int x, int y, PixelType ptype) const noexcept;
	bool classOfIs(int x, int y, PixelClass pclass) const noexcept;

	void step() noexcept;

	void renderToBuffer(std::span<std::uint8_t> buf) const noexcept;

protected:
	void resetDirtyFlags() noexcept;

	// Global fluid analysis, custom heuristics
	void fluidAnalysisStep() noexcept;

	// Global thermal analysis
	void thermalAnalysisStep() noexcept;

private:
	int _width;
	int _height;

	std::unique_ptr<PixelTag[]> _tags;
	std::unique_ptr<PixelElement[]> _elements;
};

namespace element {

// Common superclass for all elements, no special behavior
struct EmptySubsElement {
	void step(PixelWorld &world, int x, int y) noexcept {}
};

// Common superclass for solid elements
struct SolidElement : EmptySubsElement {
	// Empty for now
};

// Common superclass for fluid elements
struct FluidElement : EmptySubsElement {
	void step(PixelWorld &world, int x, int y) noexcept;
};

// Common superclass for gas elements (except air)
struct GasElement : EmptySubsElement {
	void step(PixelWorld &world, int x, int y) noexcept;
};

struct Steam : GasElement {
	PixelTag newTag() const noexcept;
	void step(PixelWorld &world, int x, int y) noexcept;
};

struct Smoke : GasElement {
	PixelTag newTag() const noexcept;
	void step(PixelWorld &world, int x, int y) noexcept;
};

struct Air : EmptySubsElement {
	PixelTag newTag() const noexcept;
};

struct Stone : SolidElement {
	PixelTag newTag() const noexcept;
};

struct Wood : SolidElement {
	PixelTag newTag() const noexcept;
	void step(PixelWorld &world, int x, int y) noexcept;

private:
	int burn_time = 0;
};

struct Copper : SolidElement {
	PixelTag newTag() const noexcept;
};

struct Sand : SolidElement {
	PixelTag newTag() const noexcept;
	void step(PixelWorld &world, int x, int y) noexcept;

protected:
	float vx = 0, vy = 0;
};

struct Water : FluidElement {
	PixelTag newTag() const noexcept;
	void step(PixelWorld &world, int x, int y) noexcept;
};

struct Oil : FluidElement {
	PixelTag newTag() const noexcept;
	void step(PixelWorld &world, int x, int y) noexcept;

private:
	int burn_time = 0;
};

struct FluidParticle : EmptySubsElement {
	PixelTag newTag() const noexcept;
	void step(PixelWorld &world, int x, int y) noexcept;

	FluidParticle() noexcept = default;
	FluidParticle(float init_vx, float init_vy, PixelElement element) noexcept;

protected:
	PixelElement element;
	float vx = 0, vy = 0;
};

} // namespace element

} // namespace wf

#endif // WFORGE_FALLSAND_H
