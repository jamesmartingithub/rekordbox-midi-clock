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

#include "LinkManager.h"
#include "ControllerMidiReader.h"
#include "MidiClock.h"
#include <regex>
#include <iostream>
#include <string>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <consoleapi.h>
#include <thread>

#define HELP_STRING "Commands:\n\tableton link:\n\t\tlink - starts ableton link\n\tmidi clock:\n\t\tclock - starts the midi clock (port defaults to LoopBe if not set)\n\t\tport - prints available midi out ports and sets the chosen port as clock output\n\t\toffset=n - set midi clock offset (-0.5 to 0.5)\n\tfader control:\n\t\tlist - displays all usbpcap interfaces\n\t\tcheck - prints the addresses of the first 10 incoming midi messages\n\t\tlearn - listens to 10 incoming midi messages and sets the most common as address\n\t\taddress=x.x.x - sets the usb address of the dj controller, found using 'check'\n\t\tread - starts reading fader positions to control tempo\n\tstop - stops ableton link and midi clock\n\thelp - prints this list of commands\n"

BOOL WINAPI ConsoleCtrlHandler(DWORD ctrl) {
	if (ctrl == CTRL_CLOSE_EVENT) {
		LinkManager::Stop();
		MidiClock::Stop();
		return TRUE;
	}
	return FALSE;
}

int main()
{
	SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
	printf(HELP_STRING);
	while (true) {
		std::string command;
		std::cin >> command;
		if (command == "stop") {
			LinkManager::Stop();
			MidiClock::Stop();
			break;
		} else if (command == "link") {
			LinkManager::Start();
		} else if (command == "list") {
			ControllerMidiReader::List();
		} else if (command == "check") {
			ControllerMidiReader::Check();
		} else if (command == "learn") {
			ControllerMidiReader::Learn();
		} else if (command.substr(0, 8) == "address=") {
			std::regex expression("\\d+.\\d+.\\d+");
			std::cmatch match;
			std::string address = command.substr(8, command.length() - 8).c_str();
			if (std::regex_match(address.c_str(), match, expression)) {
				ControllerMidiReader::SetAddress(ControllerMidiReader::StringToAddress(address));
				printf("Set controller address to %s\n", address.c_str());
			} else {
				printf("Invalid address: %s - example address: 1.8.5\n", address.c_str());
			}
		} else if (command == "read") {
			ControllerMidiReader::Read();
		} else if (command == "help") {
			printf(HELP_STRING);
		} else if (command == "clock") {
			std::thread midiClockThread(MidiClock::Run);
			midiClockThread.detach();
			std::thread midiSyncThread(MidiClock::Sync);
			midiSyncThread.detach();
		} else if (command == "port") {
			if (!MidiClock::IsRunning()) {
				unsigned int availablePorts = MidiClock::List();
				if (availablePorts > 0) {
					printf("Select a port number for midi clock out\n");
					std::string chosenPort;
					std::cin >> chosenPort;
					std::regex expression("\\d+");
					std::cmatch match;
					if (std::regex_match(chosenPort.c_str(), match, expression)) {
						int chosenPortNum = static_cast<int>(strtol(chosenPort.c_str(), nullptr, 10));
						if (chosenPortNum < availablePorts) {
							MidiClock::SetPort(chosenPortNum);
							printf("Set midi clock out port to %s\n", chosenPort.c_str());
						} else {
							printf("Selected port is out of range\n");
						}
					} else {
						printf("Selected port is not a number: %s\n", chosenPort.c_str());
					}
				} else {
					printf("No midi out ports available\n");
				}
			} else {
				printf("Midi clock is already running, cannot list ports\n");
			}
		} else if (command.substr(0, 7) == "offset=") {
			std::regex expression("-?0.\\d+");
			std::cmatch match;
			std::string value = command.substr(7, command.length() - 7).c_str();
			if (std::regex_match(value.c_str(), match, expression)) {
				if (!MidiClock::IsRunning()) {
					float valueF = std::strtof(value.c_str(), nullptr);
					if (valueF <= 0.5 && valueF >= -0.5) {
						MidiClock::SetOffset(valueF);
						printf("Midi clock offset set to %s\n", value.c_str());
					} else {
						printf("Offset out of range, must be between -0.5 and 0.5\n");
					}
				} else {
					printf("Midi clock is already running, cannot set offset\n");
				}
			} else {
				printf("Invalid offset: %s - example offset: -0.2\n", value.c_str());
			}
		} else {
			printf("Unrecognised command \"%s\"\n", command.c_str());
		}
	}
	return 0;
}
