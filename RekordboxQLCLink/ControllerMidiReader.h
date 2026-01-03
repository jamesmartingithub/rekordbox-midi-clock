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

#include <string>

class ControllerMidiReader {
public:
	struct Address {
		uint16_t busId;
		uint16_t deviceAddr;
		uint16_t endpoint;

		Address(uint16_t busId, uint16_t deviceAddr, uint16_t endpoint) {
			this->busId = busId;
			this->deviceAddr = deviceAddr;
			this->endpoint = endpoint;
		}
	};

	static void List();
	static void Check();
	static void Learn();
	static void SetAddress(Address* newAddress);
	static void Read();
	static Address* StringToAddress(std::string addressString);

private:
	static std::string ParseRecord(const std::string& bufferString, bool isListMode);
	static unsigned long long DecValue(const std::string& bufferString, int offset, int bytes);

	static Address* address;
	static bool shiftHeld;
	static uint8_t lastChangedDeck;
	static float lastFaderPercent;
};