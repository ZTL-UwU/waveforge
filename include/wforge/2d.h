#ifndef WFORGE_2D_H
#define WFORGE_2D_H

#include <array>
#include <generator>

namespace wf {

std::generator<std::array<int, 2>> tilesOnSegment(
	std::array<int, 2> start, std::array<int, 2> end
) noexcept;

std::generator<std::array<int, 2>> neighborsOf(
	std::array<int, 2> center, std::array<int, 2> size
) noexcept;

} // namespace wf

#endif // WFORGE_2D_H
