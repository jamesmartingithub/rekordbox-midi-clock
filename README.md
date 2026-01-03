# Rekordbox-Midi-Clock
(2025/2026) A midi clock for Rekordbox (or other programs that provide an ableton link connection) using Ableton Link, with optional DDJ-1000 tempo fader controls

## Setup
If you are using this to generate a midi clock for software such as QLC+, you will need to use a virtual midi driver such as LoopBe1
#### Ableton link and midi clock:
- Type the command 'link' and wait for the message "Connected to n peers" (if this does not appear, try disabling and re-enabling ableton link via the other software)
- Then optionally choose the midi out port using the 'port' command - if no port is set, it defaults to looking for LoopBe1
- Then optionally set the clock offset using the 'offset=n' command, where -0.5 <= n <= 0.5 (use this to account for latency)
- Then type the command 'clock' once connected to ableton link to start the midi clock
#### (Optional) DDJ-1000 Tempo fader control while using ableton link:
- First USBPcap must be downloaded and the folder placed in the same directory as this executable.
- Then either:
  - Type the command 'learn' and move the fader to automatically detect and set the usb address of the controller
  - Or type the command 'check' and move the fader to identify the controller address, then type 'address=x.x.x', replacing the x's with the address parts to set the address manually.
- Then type the command 'read' once connected to ableton link (use this command last as no commands can be used after)

## Technologies/Techniques Used:
- Parsing of USB URBs, intercepted from USB drivers via USBPcap
- Boost::process and Boost::asio to launch external executables and pipe stdout for parsing
- Precise midi clock using drift correction, threading, and synchronisation
