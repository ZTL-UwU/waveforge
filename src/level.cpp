#include "wforge/level.h"

namespace wf {

Level::Level(int width, int height) noexcept: fallsand(width, height) {}

void Level::step() {
	fallsand.step();
	duck.step(*this);
}

} // namespace wf
