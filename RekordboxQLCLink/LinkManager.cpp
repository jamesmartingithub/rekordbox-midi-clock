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

#include <LinkManager.h>
#include <MidiClock.h>
#include <ableton/Link.hpp>
#include <ableton/link/Sessions.hpp>
#include <ableton/link/SessionState.hpp>

ableton::Link* LinkManager::link = nullptr;
ableton::platforms::windows::Clock LinkManager::clock;

void LinkManager::Start() {
	link = new ableton::Link(128);
	link->setNumPeersCallback(LinkManager::NumPeersChanged);
	link->setTempoCallback(LinkManager::TempoChanged);
	link->enableStartStopSync(true);
	link->enable(true);
	clock = link->clock();
}

void LinkManager::Stop() {
	if (link != nullptr) {
		link->enable(false);
		link = nullptr;
	}
}

void LinkManager::ChangeTempo(float change) {
	if (link != nullptr) {
		ableton::Link::SessionState state = link->captureAppSessionState();
		state.setTempo(state.tempo() + change, clock.micros());
		link->commitAppSessionState(state);
	}
}

double LinkManager::GetTempo() {
	if (link != nullptr) {
		ableton::Link::SessionState state = link->captureAppSessionState();
		return state.tempo();
	} else {
		return 0;
	}
}

long long LinkManager::GetTime() {
	if (link != nullptr) {
		return clock.micros().count();
	} else {
		return 0;
	}
}

LinkManager::StartingBeat LinkManager::GetStartingBeat() {
	if (link != nullptr) {
		ableton::Link::SessionState state = link->captureAppSessionState();
		double beat = state.beatAtTime(clock.micros(), 4) + 1;
		int beatInt = static_cast<int>(beat);
		double decimal = beat - beatInt;
		// If the current beat is over halfway done, return time of beat after next instead
		if (decimal > 0.5) {
			beatInt++;
		}
		int timeUntilBeat = static_cast<int>((state.timeAtBeat(beatInt, 4) - clock.micros()).count() / 1000);
		return StartingBeat(beatInt, timeUntilBeat);
	} else {
		return StartingBeat(0, 0);
	}
}

long long LinkManager::GetDrift(int beat, long long time) {
	if (link != nullptr) {
		ableton::Link::SessionState state = link->captureAppSessionState();
		return state.timeAtBeat(beat, 4).count() - time;
	} else {
		return 0;
	}
}

bool LinkManager::IsRunning() {
	return link != nullptr;
}

void LinkManager::NumPeersChanged(const size_t numPeers) {
	printf(numPeers > 1 ? "Connected to %u peers\n" : "Connected to %u peer\n", numPeers);
}

void LinkManager::TempoChanged(const double bpm) {
	MidiClock::UpdateTempo(bpm);
}
