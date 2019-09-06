#pragma once
#include "pch.h"

template <std::size_t HIST_SIZE = 10, typename DELTA = double, typename CLOCK = std::chrono::high_resolution_clock >
struct fpsMeter_t {
	typename CLOCK::time_point last;
	std::array<DELTA, HIST_SIZE> history;

	void tick() {
		auto now = CLOCK::now();
		auto duration = std::chrono::duration<DELTA>(now - last).count();
		last = now;
		for (size_t i = 0; i < HIST_SIZE - 1; i++) {
			history[i] = history[i + 1];
		}
		history[HIST_SIZE - 1] = duration;
	}

	DELTA delta() {
		return history[HIST_SIZE - 1];
	}

	DELTA deltaAvg() {
		return std::accumulate(history.begin(), history.end(), (DELTA)0) / static_cast<DELTA>(HIST_SIZE);
	}

	DELTA fps() {
		return 1 / deltaAvg();
	}

	fpsMeter_t() : last(), history() {
		history.fill(0);
	}
};