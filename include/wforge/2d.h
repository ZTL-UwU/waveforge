#ifndef WFORGE_2D_H
#define WFORGE_2D_H

#include <array>
#include <generator>

namespace wf {

std::generator<std::array<int, 2>> tilesOnSegment(
	std::array<int, 2> start, std::array<int, 2> end
) noexcept;

}

#endif // WFORGE_2D_H
