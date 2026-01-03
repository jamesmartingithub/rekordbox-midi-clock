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

#include "MidiClock.h"
#include "LinkManager.h"
#define __WINDOWS_MM__
#include "RtMidi.h"

#define SYNC_WAIT 1000

std::atomic<bool> MidiClock::running = false;
int MidiClock::port = -1;
int MidiClock::offset = 0;
std::atomic<int> MidiClock::timePerTick = 20;
std::atomic<int> MidiClock::driftCorrect = 0;
MidiClock::BeatTime MidiClock::beatTime;

void MidiClock::Run() {
	if (LinkManager::IsRunning()) {
		rt::midi::RtMidiOut* midiOut;
		try {
			midiOut = new rt::midi::RtMidiOut(rt::midi::RtMidi::Api::WINDOWS_MM, "RtMidi Output Client");
			if (port == -1) {
				unsigned int ports = midiOut->getPortCount();
				bool portOpened = false;
				for (int p = 0; p < ports; p++) {
					std::string portName = midiOut->getPortName(p);
					if (portName.length() > 6 && portName.substr(0, 6) == "LoopBe") {
						midiOut->openPort(p);
						portOpened = true;
						break;
					}
				}
				if (portOpened) {
				} else {
					printf("ERROR: Could not find LoopBe port (no port set, defaulted to LoopBe)");
				}
			} else {
				midiOut->openPort(port);
			}
			std::vector<unsigned char> clockMessage;
			clockMessage.push_back(0xF8);
			// Calculate time per midi clock tick using tempo (24 ticks per beat)
			timePerTick = 60000.0 / (24 * LinkManager::GetTempo());
			// Sleep until next beat to start clock beat-aligned
			LinkManager::StartingBeat startingBeat = LinkManager::GetStartingBeat();
			int beat = startingBeat.beat;
			std::this_thread::sleep_for(std::chrono::milliseconds(startingBeat.timeUntilBeat));
			uint8_t tick = 0;
			int localDrift = 0;
			printf("Started midi clock\n");
			running = true;
			// Send 12 clock ticks to account for half-beat delay
			for (int i = 0; i < 12 + offset; i++) {
				midiOut->sendMessage(&clockMessage);
			}
			while (running) {
				midiOut->sendMessage(&clockMessage);
				// Check if start of beat
				if (tick == 0) {
					// Start of beat, record time of beat for drift calculation
					beatTime.lock();
					beatTime.beat = beat;
					beatTime.time = LinkManager::GetTime();
					beatTime.unlock();
					beat++;
				}
				tick = (tick + 1) % 24;
				// Sleep less or more depending on current drift from ableton link
				localDrift += driftCorrect.exchange(0);
				if (localDrift >= 0) {
					std::this_thread::sleep_for(std::chrono::milliseconds(timePerTick.load() + localDrift));
					localDrift = 0;
				} else {
					localDrift += timePerTick.load();
				}
			}
		} catch (rt::midi::RtMidiError& error) {
			error.printMessage();
		}
		delete(midiOut);
	} else {
		printf("ERROR: ableton link is not running\n");
	}
}

void MidiClock::Sync() {
	_sleep(SYNC_WAIT);
	while (running) {
		// Calculate drift using latest beat+time
		int beat;
		long long time;
		beatTime.lock();
		beat = beatTime.beat;
		time = beatTime.time;
		beatTime.unlock();
		int drift = static_cast<int>(LinkManager::GetDrift(beat, time)) / 1000;
		// Update driftCorrect value used by midi clock to calculated drift for correction
		driftCorrect = drift;
		_sleep(SYNC_WAIT);
	}
}

void MidiClock::Stop() {
	running = false;
}

unsigned int MidiClock::List() {
	rt::midi::RtMidiOut* midiOut;
	try {
		midiOut = new rt::midi::RtMidiOut(rt::midi::RtMidi::Api::WINDOWS_MM, "RtMidi Output Client");
		unsigned int ports = midiOut->getPortCount();
		if (ports > 0) {
			for (int port = 0; port < ports; port++) {
				printf("Port %d: %s\n", port, midiOut->getPortName(port).c_str());
			}
			delete(midiOut);
			return ports;
		}
	} catch (rt::midi::RtMidiError& error) {
		error.printMessage();
	}
	delete(midiOut);
	return 0;
}

void MidiClock::SetPort(unsigned int chosenPort) {
	port = chosenPort;
}

void MidiClock::SetOffset(float offset) {
	MidiClock::offset = 24 * offset;
}

bool MidiClock::IsRunning() {
	return running;
}

void MidiClock::UpdateTempo(double tempo) {
	if (running) {
		// Calculate time per midi clock tick using tempo (24 ticks per beat)
		timePerTick = 60000.0 / (24 * tempo);
	}
}
