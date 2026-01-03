/*
	Copyright (c) 2026 James Martin

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, see https://www.gnu.org/licenses/
*/

#pragma once

#include <atomic>
#include <mutex>

class MidiClock {
public:
	static void Run();
	static void Sync();
	static void Stop();
	static unsigned int List();
	static void SetPort(unsigned int chosenPort);
	static void SetOffset(float offset);
	static bool IsRunning();
	static void UpdateTempo(double tempo);

private:
	struct BeatTime : public std::mutex {
		int beat;
		long long time;
	};

	static std::atomic<bool> running;
	static int port;
	static int offset;
	static std::atomic<int> timePerTick;
	static std::atomic<int> driftCorrect;
	static BeatTime beatTime;
};