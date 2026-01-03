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

#ifndef LINK_MANAGER_H
#define LINK_MANAGER_H

namespace ableton {
	class Link;
	namespace platforms {
		namespace windows {
			class Clock;
		}
	}
}

class LinkManager {
public:
	struct StartingBeat {
		int beat;
		int timeUntilBeat;

		StartingBeat(int beat, int timeUntilBeat) : beat(beat), timeUntilBeat(timeUntilBeat) {}
	};

	static void Start();
	static void Stop();
	static void ChangeTempo(float change);
	static double GetTempo();
	static long long GetTime();
	static StartingBeat GetStartingBeat();
	static long long GetDrift(int beat, long long time);
	static bool IsRunning();

private:
	static void NumPeersChanged(const size_t numPeers);
	static void TempoChanged(const double bpm);

	static ableton::Link* link;
	static ableton::platforms::windows::Clock clock;
};

#endif //LINK_MANAGER_H