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

#include "ControllerMidiReader.h"
#include "LinkManager.h"
#define WIN32_LEAN_AND_MEAN
#define BOOST_PROCESS_VERSION 2
#pragma comment(lib,"ntdll.lib")
#include <boost/process/v2/process.hpp>
#include <boost/process/v2/popen.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <fcntl.h>
#include <algorithm>
#include <map>

using namespace boost;

#define BUFFER_SIZE 1024
#define USBPCAP_PATH "./USBPCap/USBPcapCMD.exe"
#define INTERFACE_1 "\\\\.\\USBPcap1"
#define INTERFACE_2 "\\\\.\\USBPcap2"

ControllerMidiReader::Address* ControllerMidiReader::address = nullptr;
bool ControllerMidiReader::shiftHeld = false;
uint8_t ControllerMidiReader::lastChangedDeck = 4;
float ControllerMidiReader::lastFaderPercent = 0;


void ControllerMidiReader::List() {
    try {
        asio::io_context ctx;
        process::v2::process proc(ctx, USBPCAP_PATH, {}, process::process_stdio{ {}, stdout, {} });
        proc.wait();
    } catch (wrapexcept<boost::system::system_error> error) {
        printf("ERROR Cannot find USBPcapCMD.exe\n");
    }
}

void ControllerMidiReader::Check() {
    try {
        asio::io_context ctx;
        std::string output;
        asio::readable_pipe rp{ ctx };

        process::v2::process proc(ctx, USBPCAP_PATH, {
        "-A",
        "-d", INTERFACE_1,
        "-d", INTERFACE_2,
        "-o", "-"
            }, process::v2::process_stdio{ {}, rp, {} });
        proc.detach();

        auto buff = asio::dynamic_buffer(output, 1024);
        // Discard global header
        asio::read(rp, buff, boost::asio::transfer_exactly(24));
        int printCount = 0;
        while (printCount < 10) {
            // Read libcap record header
            buff.consume(buff.size());
            asio::read(rp, buff, boost::asio::transfer_exactly(16));
            int recordLength = (int)DecValue(output, 8, 4);
            // Check if record can fit in buffer
            if (recordLength > BUFFER_SIZE) {
                // Record too large for buffer, read and ignore
                while (recordLength > 0) {
                    int skipAmount = min(recordLength, BUFFER_SIZE);
                    buff.consume(buff.size());
                    asio::read(rp, buff, boost::asio::transfer_exactly(skipAmount));
                    recordLength -= skipAmount;
                }
                continue;
            } else {
                buff.consume(buff.size());
                asio::read(rp, buff, boost::asio::transfer_exactly(recordLength));
                // Parse record data
                if (ParseRecord(output, true) != "") {
                    printCount++;
                }
            }
        }
        printf("Check finished\n");
    } catch (wrapexcept<boost::system::system_error> error) {
        printf("ERROR Cannot find USBPcapCMD.exe\n");
    }
}

void ControllerMidiReader::Learn() {
    try {
        std::map<std::string, int> addresses{};

        asio::io_context ctx;
        std::string output;
        asio::readable_pipe rp{ ctx };

        process::v2::process proc(ctx, USBPCAP_PATH, {
        "-A",
        "-d", INTERFACE_1,
        "-d", INTERFACE_2,
        "-o", "-"
            }, process::v2::process_stdio{ {}, rp, {} });
        proc.detach();

        auto buff = asio::dynamic_buffer(output, 1024);
        // Discard global header
        asio::read(rp, buff, boost::asio::transfer_exactly(24));
        // Only output 10 addresses
        int printCount = 0;
        while (printCount < 10) {
            // Read libcap record header
            buff.consume(buff.size());
            asio::read(rp, buff, boost::asio::transfer_exactly(16));
            int recordLength = (int)DecValue(output, 8, 4);
            // Check if record can fit in buffer
            if (recordLength > BUFFER_SIZE) {
                // Record too large for buffer, read and ignore
                while (recordLength > 0) {
                    int skipAmount = min(recordLength, BUFFER_SIZE);
                    buff.consume(buff.size());
                    asio::read(rp, buff, boost::asio::transfer_exactly(skipAmount));
                    recordLength -= skipAmount;
                }
                continue;
            } else {
                buff.consume(buff.size());
                asio::read(rp, buff, boost::asio::transfer_exactly(recordLength));
                // Parse record data
                std::string parseResult = ParseRecord(output, true);
                if (parseResult != "") {
                    // Address found, add to map and count
                    if (addresses.count(parseResult)) {
                        addresses[parseResult] = addresses[parseResult] + 1;
                    } else {
                        addresses.insert({parseResult, 1});
                    }
                    printCount++;
                }
            }
        }
        // Find most common address
        std::string mostCommonAddress;
        int largestCount = 0;
        for (std::pair<std::string, int> addr : addresses) {
            if (addr.second > largestCount) {
                mostCommonAddress = addr.first;
            }
        }
        // Set address to most common
        address = StringToAddress(mostCommonAddress);
        printf("Set controller address to %s\n", mostCommonAddress.c_str());
    } catch (wrapexcept<boost::system::system_error> error) {
        printf("ERROR Cannot find USBPcapCMD.exe\n");
    }
}

void ControllerMidiReader::SetAddress(Address* newAddress) {
    address = newAddress;
}

void ControllerMidiReader::Read() {
    if (address == nullptr) {
        printf("Unable to start read, controller address not set (use command 'learn' or 'address' first)\n");
    } else {
        try {
            asio::io_context ctx;
            std::string output;
            asio::readable_pipe rp{ ctx };

            process::v2::process proc(ctx, USBPCAP_PATH, {
            "-A",
            "-d", (address->busId == 1) ? INTERFACE_1 : INTERFACE_2,
            "-o", "-"
            }, process::v2::process_stdio{ nullptr, rp, nullptr });
            proc.detach();

            auto buff = asio::dynamic_buffer(output, 1024);
            // Discard global header
            asio::read(rp, buff, boost::asio::transfer_exactly(24));
            while (true) {
                // Read libcap record header
                buff.consume(buff.size());
                asio::read(rp, buff, boost::asio::transfer_exactly(16));
                int recordLength = (int)DecValue(output, 8, 4);
                // Check if record can fit in buffer
                if (recordLength > BUFFER_SIZE) {
                    // Record too large for buffer, read and ignore
                    while (recordLength > 0) {
                        int skipAmount = min(recordLength, BUFFER_SIZE);
                        buff.consume(buff.size());
                        asio::read(rp, buff, boost::asio::transfer_exactly(skipAmount));
                        recordLength -= skipAmount;
                    }
                    continue;
                } else {
                    buff.consume(buff.size());
                    asio::read(rp, buff, boost::asio::transfer_exactly(recordLength));
                    // Parse record data
                    ParseRecord(output, false);
                }
            }
            printf("Read stopped\n");
        } catch (wrapexcept<boost::system::system_error> error) {
            printf("ERROR Cannot find USBPcapCMD.exe\n");
        }
    }
}

ControllerMidiReader::Address* ControllerMidiReader::StringToAddress(std::string addressString) {
    int firstDotIndex = addressString.find('.');
    int secondDotIndex = addressString.substr(firstDotIndex + 1, addressString.length() - (firstDotIndex + 1)).find('.') + firstDotIndex + 1;
    uint16_t busId = static_cast<uint16_t>(strtol(addressString.substr(0, firstDotIndex).c_str(), nullptr, 10));
    uint16_t deviceAddr = static_cast<uint16_t>(strtol(addressString.substr(firstDotIndex + 1, secondDotIndex - (firstDotIndex + 1)).c_str(), nullptr, 10));
    uint16_t endpoint = static_cast<uint16_t>(strtol(addressString.substr(secondDotIndex + 1, addressString.length() - (secondDotIndex + 1)).c_str(), nullptr, 10));
    return new ControllerMidiReader::Address(busId, deviceAddr, endpoint);
}

std::string ControllerMidiReader::ParseRecord(const std::string& bufferString, bool isListMode) {
    // Parse packet header fields
    uint16_t urbFunction = static_cast<uint16_t>(DecValue(bufferString, 14, 2));
    if (urbFunction == 9) {
        // URB function is bulk or interrupt transfer
        uint16_t headerLength = static_cast<uint16_t>(DecValue(bufferString , 0, 2));
        uint32_t dataLength = static_cast<uint32_t>(DecValue(bufferString, 23, 4));
        uint16_t urbBusId = static_cast<uint16_t>(DecValue(bufferString, 17, 2));
        uint16_t deviceAddress = static_cast<uint16_t>(DecValue(bufferString, 19, 2));
        uint8_t endpointAndTransferDir = static_cast<uint8_t>(bufferString[21]);
        uint8_t endpoint;
        uint8_t direction; // IN=1 OUT=0
        if (endpointAndTransferDir >= 128) {
            // EndpointAndTransferDir most significant bit is 1, set to 0
            endpoint = endpointAndTransferDir ^ 128;
            direction = 1;
        } else {
            endpoint = endpointAndTransferDir;
            direction = 0;
        }
        uint8_t transferType = endpointAndTransferDir = static_cast<uint8_t>(bufferString[22]);
        if (transferType == 3 && direction == 1 && dataLength >= 4 && dataLength % 4 == 0) {
            // URB transfer type is bulk and direction is IN
            if (isListMode) {
                // List argument given, output full address
                printf("[%u.%u.%u]\n", urbBusId, deviceAddress, endpoint);
                std::ostringstream addressStream;
                addressStream << urbBusId << "." << deviceAddress << "." << static_cast<uint16_t>(endpoint);
                return addressStream.str();
            } else {
                // Address argument given, check full address matches requested
                if (address->busId == urbBusId && address->deviceAddr == deviceAddress && address->endpoint == endpoint) {
                    // Could be tempo value, check status and data1 fields
                    uint8_t status = static_cast<uint8_t>(bufferString[headerLength + 1]);
                    uint8_t statusMSB = status & 0xF0;
                    uint8_t statusLSB = status & 0x0F;
                    if (statusMSB == 0xB0 && statusLSB >= 0 && statusLSB <= 3) {
                        // CC type
                        // Multiple midi messages can be joined, check each length of 8 for a fader value
                        for (int i = 0; i + 8 <= dataLength; i+=8) {
                            if ((static_cast<uint8_t>(bufferString[headerLength + 2 + i]) == 0x00 ||
                                static_cast<uint8_t>(bufferString[headerLength + 2 + i]) == 0x05) && // data1MSB
                                (static_cast<uint8_t>(bufferString[headerLength + 6 + i]) == 0x20 ||
                                    static_cast<uint8_t>(bufferString[headerLength + 6 + i]) == 0x25)) { // data1LSB
                                // Fader move
                                // Calculate fader value from MSB and LSB
                                uint16_t data2MSB = static_cast<uint16_t>(bufferString[headerLength + 3 + i]);
                                uint16_t data2LSB = static_cast<uint16_t>(bufferString[headerLength + 7 + i]);
                                uint16_t faderVal = (data2MSB << 8) | data2LSB;
                                float faderPercent = roundf(faderVal / 200.0) / 10.0; //327
                                if (lastChangedDeck != statusLSB) {
                                    lastChangedDeck = statusLSB;
                                } else {
                                    // Ignore if tempo held
                                    if (!shiftHeld) {
                                        if (faderPercent != lastFaderPercent) {
                                            // Send tempo change to link manager
                                            LinkManager::ChangeTempo(lastFaderPercent - faderPercent);
                                        }
                                    }
                                }
                                lastFaderPercent = faderPercent;
                                break;
                            }
                        }
                    } else if (statusMSB == 0x90 && statusLSB >= 0 && statusLSB <= 3) {
                        // NOTE type
                        // Multiple midi messages can be joined, check each length of 4 for a shift value
                        for (int i = 0; i < dataLength / 4; i+=4) {
                            if (static_cast<uint8_t>(bufferString[headerLength + 2 + i]) == 0x3F) { // data1
                                // Shift press or release
                                if (static_cast<uint16_t>(bufferString[headerLength + 3 + i]) == 0x7F) { // data2
                                    // Shift ON
                                    shiftHeld = true;
                                } else {
                                    // Shift OFF
                                    shiftHeld = false;
                                }
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
    return "";
}

unsigned long long ControllerMidiReader::DecValue(const std::string& bufferString, int offset, int bytes) {
    unsigned long long decValue = 0;
    for (int i = bytes - 1; i >= 0; i--) {
        decValue = (decValue << 8) | static_cast<unsigned long long>(static_cast<uint8_t>(bufferString[offset + i]));
    }
    return decValue;
}
