#ifndef WFORGE_FALLSAND_H
#define WFORGE_FALLSAND_H

#include "wforge/debug.h"
#include <cstdint>
#include <memory>
#include <proxy/proxy.h>
#include <proxy/v4/proxy_macros.h>

namespace wf {

namespace _dispatch {

PRO_DEF_MEM_DISPATCH(MemHash, hash);
PRO_DEF_MEM_DISPATCH(MemDefualtTag, defaultTag);
PRO_DEF_MEM_DISPATCH(MemStep, step);

} // namespace _dispatch

struct PixelTag;
class PixelWorld;

/* clang-format off */
struct PixelFacade : pro::facade_builder
	::add_convention<_dispatch::MemHash, std::size_t() const noexcept>
	::add_convention<_dispatch::MemDefualtTag, PixelTag() const noexcept>
	::add_convention<_dispatch::MemStep, void(PixelWorld &world, int x, int y) noexcept>
	::build {};
/* clang-format on */

using PixelElement = pro::proxy<PixelFacade>;

enum class PixelType : std::uint8_t {
	Air,
	Stone,
	Sand,
	Water,
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
};

class PixelWorld {
public:
	PixelWorld();
	PixelWorld(int width, int height);

	int width() const noexcept {
		return _width;
	}

	int height() const noexcept {
		return _height;
	}

	int rand() noexcept;

	PixelTag tagOf(int x, int y) const noexcept;
	PixelTag &tagOf(int x, int y) noexcept;
	PixelElement &elementOf(int x, int y) noexcept;

	void swapPixels(int x1, int y1, int x2, int y2) noexcept;
	void replacePixel(int x, int y, PixelElement new_pixel) noexcept;
	void replacePixelWithAir(int x, int y) noexcept;

	bool typeOfIs(int x, int y, PixelType ptype) const noexcept;
	bool classOfIs(int x, int y, PixelClass pclass) const noexcept;

	void step() noexcept;

private:
	int _width;
	int _height;

	std::unique_ptr<PixelTag[]> _tags;
	std::unique_ptr<PixelElement[]> _elements;
};

namespace element {

struct AbstractElement {
	void step(PixelWorld &world, int x, int y) noexcept {}
};

struct Air : AbstractElement {
	std::size_t hash() const noexcept;
	PixelTag defaultTag() const noexcept;
};

struct Stone : AbstractElement {
	std::size_t hash() const noexcept;
	PixelTag defaultTag() const noexcept;
};

struct Sand {
	std::size_t hash() const noexcept;
	PixelTag defaultTag() const noexcept;
	void step(PixelWorld &world, int x, int y) noexcept;
};

struct Water {
	std::size_t hash() const noexcept;
	PixelTag defaultTag() const noexcept;
	void step(PixelWorld &world, int x, int y) noexcept;

	int dir;
};

} // namespace element

} // namespace wf

#endif // WFORGE_FALLSAND_H
