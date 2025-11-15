#include "wforge/elements.h"

namespace wf {

PixelElement constructElementByType(PixelType type) noexcept {
	switch (type) {
	case PixelType::Air:
		return element::Air::create();

	case PixelType::Decoration:
		return element::Decoration::create();

	case PixelType::Stone:
		return element::Stone::create();

	case PixelType::Wood:
		return element::Wood::create();

	case PixelType::Copper:
		return element::Copper::create();

	case PixelType::Sand:
		return element::Sand::create();

	case PixelType::Water:
		return element::Water::create();

	case PixelType::Oil:
		return element::Oil::create();

	default:
		return element::Decoration::create();
	}
}

} // namespace wf
