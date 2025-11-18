#ifndef WFORGE_ELEMENTS_H
#define WFORGE_ELEMENTS_H

#include "wforge/fallsand.h"

namespace wf {

PixelElement constructElementByType(PixelType type) noexcept;

namespace element {

// Common superclass for all elements, no special behavior
struct EmptySubsElement {
	void step(PixelWorld &world, int x, int y) noexcept {}
	void onCharge(PixelWorld &world, int x, int y) noexcept {}
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

	static PixelElement create() noexcept;
};

struct Decoration : SolidElement {
	PixelTag newTag() const noexcept;

	static PixelElement create() noexcept;
};

struct Stone : SolidElement {
	PixelTag newTag() const noexcept;

	static PixelElement create() noexcept;
};

struct Wood : SolidElement {
	Wood() noexcept;

	PixelTag newTag() const noexcept;
	void step(PixelWorld &world, int x, int y) noexcept;

	static PixelElement create() noexcept;

private:
	int burn_time_left;
};

struct Copper : SolidElement {
	PixelTag newTag() const noexcept;
	void onCharge(PixelWorld &world, int x, int y) noexcept;
	void step(PixelWorld &world, int x, int y) noexcept;

	static PixelElement create() noexcept;
};

struct Sand : SolidElement {
	PixelTag newTag() const noexcept;
	void step(PixelWorld &world, int x, int y) noexcept;

	static PixelElement create() noexcept;

protected:
	float vx = 0, vy = 0;
};

struct Water : FluidElement {
	PixelTag newTag() const noexcept;
	void step(PixelWorld &world, int x, int y) noexcept;

	static PixelElement create() noexcept;
};

struct Oil : FluidElement {
	Oil() noexcept;

	PixelTag newTag() const noexcept;
	void step(PixelWorld &world, int x, int y) noexcept;

	static PixelElement create() noexcept;

private:
	int burn_time_left;
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

#endif // WFORGE_ELEMENTS_H
