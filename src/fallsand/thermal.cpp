#include "wforge/fallsand.h"
#include "wforge/xoroshiro.h"
#include <algorithm>
#include <cmath>
#include <condition_variable>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

namespace wf {

namespace {

constexpr float heat_transfer_factor = 0.15f;
constexpr float heat_decay_factor = 0.005f;
constexpr std::uint64_t rng_max = Xoroshiro128PP::max();

// Why 4 instead of std::thread::hardware_concurrency()?
// After all threads done their work, the main thread needs to merge
// the results. If there are too many threads, the merging step becomes
// a bottleneck. 2-6 seems to be a good compromise for most systems.
// This value can be adjusted later based on profiling results.
constexpr int num_thermal_analysis_workers = 4;

// Work phase enumeration
enum class WorkPhase {
	Idle,
	HeatTransfer,
	HeatDecay
};

// Thermal analysis worker thread
class ThermalWorker {
public:
	ThermalWorker(int worker_id)
		: _worker_id(worker_id)
		, _rng(Seed::device_random())
		, _phase(WorkPhase::Idle) {
		_thread = std::jthread([this](std::stop_token stoken) {
			workerLoop(stoken);
		});
	}

	// Non-copyable and non-movable
	ThermalWorker(const ThermalWorker &) = delete;
	ThermalWorker &operator=(const ThermalWorker &) = delete;

	void startWork(
		WorkPhase phase, const PixelWorld *world, int width, int height,
		int y_start, int y_end, std::vector<int> heat_maps[]
	) {
		{
			std::lock_guard<std::mutex> lock(_work_mutex);
			_phase = phase;
			_world = world;
			_width = width;
			_height = height;
			_y_start = y_start;
			_y_end = y_end;
			_heat_maps = heat_maps;
			_work_ready = true;
		}
		_cv.notify_one();
	}

	void waitForCompletion() {
		std::unique_lock<std::mutex> lock(_work_mutex);
		_cv_done.wait(lock, [this] {
			return !_work_ready;
		});
	}

	~ThermalWorker() noexcept {
		_thread.request_stop();
		_cv.notify_one();
		_thread.join();
	}

private:
	void workerLoop(std::stop_token stoken) {
		while (!stoken.stop_requested()) {
			try {
				std::unique_lock<std::mutex> lock(_work_mutex);
				_cv.wait(lock, [this, &stoken] {
					return _work_ready || stoken.stop_requested();
				});

				if (stoken.stop_requested()) {
					break;
				}

				WorkPhase current_phase = _phase;
				lock.unlock();

				// Execute actual work
				if (current_phase == WorkPhase::HeatTransfer) {
					doHeatTransfer();
				} else if (current_phase == WorkPhase::HeatDecay) {
					doHeatDecay();
				}

				lock.lock();
				_work_ready = false;
				lock.unlock();
				_cv_done.notify_one();
			} catch (const std::exception &e) {
				std::cerr << "Fatal error in thermal worker " << _worker_id
						  << ": " << e.what() << std::endl;
				std::abort();
			} catch (...) {
				std::cerr << "Fatal unknown error in thermal worker "
						  << _worker_id << std::endl;
				std::abort();
			}
		}
	}

	void doHeatTransfer() {
		constexpr int dx[] = {-1, 1, 0, 0};
		constexpr int dy[] = {0, 0, -1, 1};
		int conductivity_weights[4];

		for (int y = _y_start; y < _y_end; ++y) {
			for (int x = 0; x < _width; ++x) {
				auto tag = _world->tagOf(x, y);

				if (tag.heat == 0 || tag.thermal_conductivity == 0) {
					_heat_maps[_worker_id][y * _width + x] += tag.heat;
					continue;
				}

				float total_transfer_amount = 0;

				int total_thermal_conductivity = std::round(
					tag.heat
					* (PixelTag::thermal_conductivity_max
				       - tag.thermal_conductivity)
					/ heat_transfer_factor
				);

				for (int i = 0; i < 4; ++i) {
					int nx = x + dx[i];
					int ny = y + dy[i];
					if (nx < 0 || nx >= _width || ny < 0 || ny >= _height) {
						conductivity_weights[i] = 0;
						continue;
					}

					auto ntag = _world->tagOf(nx, ny);
					auto delta_heat = std::max<int>(0, tag.heat - ntag.heat);
					auto relative_conductivity = std::min(
						tag.thermal_conductivity, ntag.thermal_conductivity
					);

					conductivity_weights[i] = delta_heat
						* relative_conductivity;
				}

				for (int i = 0; i < 4; ++i) {
					int nx = x + dx[i];
					int ny = y + dy[i];
					if (conductivity_weights[i] == 0) {
						continue;
					}

					auto ntag = _world->tagOf(nx, ny);
					int conductivity = conductivity_weights[i];

					float transfer_amount = 1.f * tag.heat * conductivity
						/ total_thermal_conductivity;

					int received_heat = std::floor(transfer_amount);
					float frac = (transfer_amount - received_heat) / 2;
					if (_rng()
					    < std::round(frac * static_cast<double>(rng_max))) {
						received_heat += 1;
					}

					total_transfer_amount += transfer_amount;
					_heat_maps[_worker_id][ny * _width + nx] += received_heat;
				}
				_heat_maps[_worker_id][y * _width + x] += tag.heat
					- std::round(total_transfer_amount);
			}
		}
	}

	void doHeatDecay() {
		const int left = _y_start * _width;
		const int right = _y_end * _width;

		// Merge all worker heat maps into the first worker's heat map
		for (int worker_id = 1; worker_id < num_thermal_analysis_workers;
		     ++worker_id) {
			for (int i = left; i < right; ++i) {
				_heat_maps[0][i] += _heat_maps[worker_id][i];
			}
		}

		for (int i = left; i < right; ++i) {
			auto &next_heat = _heat_maps[0][i];
			if (next_heat <= 0) {
				continue;
			}

			float delta = next_heat * heat_decay_factor;
			int nat = std::floor(delta);
			float frac = delta - nat;
			next_heat -= nat;
			if (next_heat > 0
			    && _rng() < std::round(frac * static_cast<double>(rng_max))) {
				next_heat -= 1;
			}

			// Clamp to valid range
			next_heat = std::clamp<int>(next_heat, 0, PixelTag::heat_max);
		}
	}

	int _worker_id;
	Xoroshiro128PP _rng;
	std::jthread _thread;
	std::mutex _work_mutex;
	std::condition_variable _cv;
	std::condition_variable _cv_done;
	bool _work_ready = false;

	// Work parameters
	WorkPhase _phase;
	const PixelWorld *_world = nullptr;
	int _width = 0;
	int _height = 0;
	int _y_start = 0;
	int _y_end = 0;
	std::vector<int> *_heat_maps = nullptr;
};

// Thread pool manager
class ThermalWorkerPool {
public:
	ThermalWorkerPool() {
		for (int i = 0; i < num_thermal_analysis_workers; ++i) {
			_workers.emplace_back(std::make_unique<ThermalWorker>(i));
		}
	}

	void executeHeatTransfer(const PixelWorld *world, int width, int height) {
		const int rows_per_worker = (height + num_thermal_analysis_workers - 1)
			/ num_thermal_analysis_workers;

		// Reset and resize heat maps
		for (auto &heat_map : _worker_heat_maps) {
			heat_map.assign(width * height, 0);
		}

		// Distribute tasks
		for (int i = 0; i < num_thermal_analysis_workers; ++i) {
			int y_start = i * rows_per_worker;
			int y_end = std::min(y_start + rows_per_worker, height);
			_workers[i]->startWork(
				WorkPhase::HeatTransfer, world, width, height, y_start, y_end,
				_worker_heat_maps.data()
			);
		}

		// Wait for completion
		for (auto &worker : _workers) {
			worker->waitForCompletion();
		}
	}

	void executeHeatDecay(const PixelWorld *world, int width, int height) {
		const int rows_per_worker = (height + num_thermal_analysis_workers - 1)
			/ num_thermal_analysis_workers;

		// Distribute tasks
		for (int i = 0; i < num_thermal_analysis_workers; ++i) {
			int y_start = i * rows_per_worker;
			int y_end = std::min(y_start + rows_per_worker, height);
			_workers[i]->startWork(
				WorkPhase::HeatDecay, world, width, height, y_start, y_end,
				_worker_heat_maps.data()
			);
		}

		// Wait for completion
		for (auto &worker : _workers) {
			worker->waitForCompletion();
		}
	}

	const std::vector<int> &getResults() const {
		return _worker_heat_maps[0];
	}

private:
	std::vector<std::unique_ptr<ThermalWorker>> _workers;

	// It seems that there is no data-race for heat maps since each worker
	// only writes to its own rows. Why not just allocate a single heat map
	// instead of having one per worker?
	// Well, having one per worker avoids false sharing and cache
	// contention, which, although consumes more memory, can significantly
	// improve performance. The memory usage is not that large anyway. Only
	// ~200KB per worker for a standard world. Closing a chrome tab frees much
	// more memory than that :)
	std::array<std::vector<int>, num_thermal_analysis_workers>
		_worker_heat_maps;
};

// Global thread pool instance
ThermalWorkerPool &getThermalWorkerPool() {
	static ThermalWorkerPool pool;
	return pool;
}

} // namespace

void PixelWorld::thermalAnalysisStep() noexcept {
	auto &pool = getThermalWorkerPool();

	// Heat transfer - parallel computation using thread pool
	pool.executeHeatTransfer(this, _width, _height);

	// Heat decay - parallel computation using thread pool
	pool.executeHeatDecay(this, _width, _height);

	// Apply final results
	const auto &next_heat = pool.getResults();
	for (int i = 0; i < _width * _height; ++i) {
		_tags[i].heat = next_heat[i];
	}
}

} // namespace wf
