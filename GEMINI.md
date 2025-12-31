This GEMINI.md file is structured specifically for a developer building a C# MIDI editor.
This is a system to graphically edit the content of a Roland GP-16 using MIDI SysEx messages
It maps out the hardware architecture, the SysEx protocol requirements, and the parameter data needed to build the UI and communication logic

# General Code Guidance
* Don't write comments, only the generated code
* The system is implemented using C#
* Consider all PropertyChanged events as nullable
* Don't do any git updates

# C# setup
* Use .NET MAUI as the graphical library
* Use C# Dev Kit to manage the application build
* Use Melanchall.DryWetMidi to send and receiev SysEx messages

# MIDI & SysEx Implementation
* When sending SysEx, the device require a small delay (typically 20ms–50ms) between large messages to prevent buffer overflow on the device side.
* Use Roland SysEx checksum calculations

To edit the GP-16 in real-time, the editor must use the Roland Checksum-protected SysEx protocol.
## SysEx Message Structure

Standard Roland 1-way communication format:F0 41 [Device_ID] 2A 12 [Address] [Data] [Checksum] F7
* Manufacturer ID: 41 (Roland)
* Device ID: Default is 10 (Adjustable in System settings 00-1F).
* Model ID: 2A (GP-16 specific).
* Command ID: 12 (Data Set / Receive).

## The Checksum Formula
Roland uses a 7-bit checksum.
1. Sum the Address and Data bytes.
2. Divide by 128 and keep the remainder.
3. Subtract the remainder from 128.
4. If the result is 128, the checksum is 0.

## Patch Management: Sending and Receiving

The GP-16 uses **Roland System Exclusive (SysEx)** messages for bulk data transfers and real-time editing.

### Bulk Dump (Sending Patches)
Modes: You can transmit data for a 
* single Number (1 patch)
* a Bank (8 patches)
* a Group (64 patches)
* All (128 patches)
* Temp (the currently selected buffer).

Temporary Data: When a patch is selected, it is copied into a "Sound-use data region" (Temporary Buffer). 
Sending "TEMP" data allows for live editing without immediately overwriting stored memory.

### Bulk Load (Receiving Patches)
Data Load: Prepares the GP-16 to receive external MIDI data and store it in internal memory.
Verify Function: The GP-16 can compare received data against its internal memory to check for errors. If successful, it displays "VERIFY OK!".

### MIDI Setup Requirements

Channels: The MIDI Transmit and Receive channels on both devices must match.
Unit Number: For SysEx communication, the **Unit Number** (Device ID) is derived from the MIDI Channel number (e.g., Channel 1 uses Device ID 00H).
Protocol: Uses the One-way communication format (Roland Exclusive Type IV).
Command IDs: `11H` (Request Data - RQ1) and `12H` (Data Set - DT1).
Model ID: The Model ID for the GP-16 is `2AH`.
Checksum: Roland's standard 7-bit checksum is required for all data packets.

#### Roland System Exlusive Messages
Roland Exclusive messages are used to transfer sound data or settings between the GP-16 and other MIDI devices (such as another GP-16 or a computer). 
These messages use a specific format that includes a Manufacturer ID, Device ID (Unit Number), and a Checksum to ensure data integrity.

1. Data Format
The Roland Exclusive message format follows this sequence:
Byte,Description,Value
F0H,Exclusive Status,Start of System Exclusive message
41H,Manufacturer ID,Roland ID
dev,Device ID,Unit Number (MIDI Channel - 1)
2AH,Model ID,GP-16 Specific ID
cmd,Command ID,11H (RQ1) or 12H (DT1)
[body],Main Data,Address and Data bytes
sum,Checksum,Error correction byte
F7H,EOX,End of Exclusive message

2. Commands (Command ID)
The GP-16 recognizes two primary commands for data communication:

Request Data 1 (RQ1) — 11
HThis command is sent to the GP-16 to request it to transmit a specific set of data. Upon receiving this, the GP-16 checks the address and size. If they are valid, it transmits the requested data using a "Data Set 1" message.

Data Set 1 (DT1) — 12HThis command carries the actual data. It is used in two ways:
From External Device to GP-16: To change settings or load patches.
From GP-16 to External Device: In response to an RQ1 command or during a manual Bulk Dump.

3. Address and Data Size
Data within the GP-16 is managed in a virtual memory map. To access specific parameters, you must provide the correct Address and the Size (number of bytes).

Addresses: Expressed in 3 bytes (7-bit format).
Size: Also expressed in 3 bytes (7-bit format).[!IMPORTANT]Roland addresses are 7-bit values $00-$7F. When calculating an address, if a value exceeds 127, it carries over to the next significant byte.

4. One-Way Communication

The GP-16 primarily uses one-way communication for simple tasks like real-time parameter editing.

Header: F0 41 10 2A 12 (assuming Device ID 10H)
Address: 3 bytes indicating which parameter to change.Data: The new value for the parameter.
Checksum: 1 byte.
Footer: F7

6. Handshaking Communication (Advanced)
Note: While the GP-16 supports simple one-way transfers, handshaking is used for large bulk dumps where the receiving device must confirm it is ready for the next packet.
ACK (Acknowledgment): 43H
NAK (Negative Acknowledgment): 4EH
ERR (Error): 4FH
WANT (Wait): 48H

If the GP-16 receives a "WANT" message, it will pause transmission until it receives an "ACK". If it receives "NAK" or "ERR", it will re-transmit the last packet.

7. Address Map Summary (Typical Offsets)

Detailed parameter addresses are found in the MIDI Implementation Chart section.
Address (Hex),Description
00 00 00,Temporary Patch Buffer (The currently running patch)
01 00 00,Internal Patch Group A (Patches 1–64)
02 00 00,Internal Patch Group B (Patches 64–128)
04 00 00,"System Settings (MIDI Channel, Master Level, etc.)

You need the Address Map offsets. In Roland SysEx, you don't send the "effect name"; you send data to a specific memory address that corresponds to a knob or switch.

The GP-16 address is composed of 3 bytes: [High] [Mid] [Low].

7.1. Base Addresses
When editing, you should almost always target the Temporary Buffer. Changes sent here are heard instantly but not saved until you send a "Write" command or the user saves on the front panel.

Area,Base Address (Hex),Description
Temporary Buffer,00 00 00,"The ""Work Area"" (Active Sound)"
Internal Group A,01 00 00,Patches 1–64
Internal Group B,02 00 00,Patches 65–128
System,04 00 00,"Global settings (MIDI Channel, etc.)"

7.2. Effect Parameter Offsets (Temporary Buffer)
To calculate the final address for a SysEx message, add the Offset below to the Temporary Buffer base (00 00 00).

Block A: Pre-Effects
Parameter,Offset (Hex),Range (Dec),Notes
A-1 Compressor,,,
Sustain,00 00 06,0–100,
Attack,00 00 07,0–100,
A-2 Distortion/Overdrive,,,
Drive,00 00 0B,0–100,
Turbo,00 00 0C,0–1,"0 = Off, 1 = On"
A-3 Picking Filter,,,
Cutoff Freq,00 00 11,0–100,
Up/Down,00 00 13,0–1,"0 = Down, 1 = Up"

Block B: Time-Based Effects
Parameter,Offset (Hex),Range (Dec),Notes
B-2a Chorus,,,
Pre-Delay,00 00 23,0–100,
Rate,00 00 24,0–100,
Depth,00 00 25,0–100,
B-5 Reverb,,,
Reverb Time,00 00 3D,0–127,High values = longer decay
Reverb Type,00 00 3F,0–9,Selects Room1 through Spring

7.3. Patch Name Address
If you want to display the name of the current patch in your C# UI, the name starts at the very beginning of the patch data.
Address: 00 00 00 through 00 00 0F
Format: ASCII (16 characters)

7.4. Constructing the "Parameter Change" Message
If you want to turn the Distortion Turbo ON (Value 01) via your C# app:
Address: 00 00 0C
Data: 01
Checksum: Sum (00+00+0C+01) = 0D. $128 - 13 = 115$ (73H).
Message: F0 41 10 2A 12 00 00 0C 01 73 F7

7.5. Bulk Data Request (The "Sync" Button)
To pull the entire current patch from the GP-16 into your editor, send a Request Data (RQ1):
Address: 00 00 00
Size: 00 00 46 (Requests all 70 bytes of the patch)
Message: F0 41 10 2A 11 00 00 00 00 00 46 [Checksum] F7
The GP-16 will respond with a DT1 message containing all parameter bytes in sequence, which you can then parse to update your UI sliders.

# GP-16 Setup
The Roland GP-16 is a 24-bit internal processing rackmount multi-effects processor. 
It features 16 effect units, of which 12 can be used simultaneously.

## Hardware Architecture
A/D Conversion: 16-bit (64x oversampling).
D/A Conversion: 16-bit (4x oversampling).
Memory: 128 Patches (Group A: 1-64, Group B: 1-64).
Interface: MIDI In/Out/Thru + FC-100 Control Port.

## Patch Structure & Signal Chain
A GP-16 patch is defined by its Effect Sequence and the individual Parameter Values.

### Effect Order (Chain)
The GP-16 allows the user to define the sequence of effects. 
In a graphical editor, this should be represented as a draggable list or a flow-chart.
* Effect Groups: Effects are categorized (A-1 to B-6).
* Variations: Some slots (like A-2) are exclusive (e.g., you choose either Distortion or Overdrive).

### Effect Parameters Reference
The GP-16 effects are organized into two blocks (A and B). Parameters generally range from **0 to 100** unless otherwise noted.

Use this table to build your UI sliders and dropdowns.

ID,Effect Name,Primary Parameters,Range/Notes
A-1,Compressor,"Sustain, Attack, Level",0-100
A-2a,Distortion,"Drive, Turbo (On/Off), Tone, Level",Analog-digital hybrid
A-2b,Overdrive,"Drive, Turbo (On/Off), Tone, Level",
A-3,Picking Filter,"Sens, Cutoff, Q, Up/Down",Dynamic wah/filter
A-4,Step Phaser,"Rate, Depth, Resonance, Step Rate",LFO-based phaser
A-5,Parametric EQ,"Low/High Gain, Mid Freq, Mid Q, Mid Gain",
A-6,Noise Suppressor,"Threshold, Release",
B-1,Short Delay,"Delay Time, Feedback, Mod Rate/Depth",0.1ms - 40ms
B-2a,Chorus,"Rate, Depth, Pre-delay, Feedback",Stereo/Mono
B-2b,Flanger,"Rate, Depth, Manual, Resonance",
B-2c,Pitch Shifter,"Coarse (-12 to +12), Fine, Pre-delay, Feedback",
B-2d,Space-D,Mode (1-4),Dimension style spatializer
B-3,Auto Panpot,"Rate, Depth, Waveform",
B-4,Tap Delay,"Delay Time (L/R/C), Feedback",Tempo-based sync
B-5,Reverb,"Type (Room, Hall, Plate), Time, Pre-delay, HF Damp",
B-6,Lineout Filter,"Type (Stack, Combo, etc.)",Pre-dated Cab Sims


#### Block A: Signal Processing Effects

A-1 Compressor:
* Sustain: Range 0–100.
* Attack: Range 0–100.
* Tone: Range -50 to +50.

* Level: Range 0–100.

A-2a/b Distortion & Overdrive:
* Distortion/Drive: Range 0–100.
* Turbo (Overdrive only): On/Off.
* Tone: Range -50 to +50.
* Level: Range 0–100.

A-3 Picking Filter:
* Sensitivity: Range 0–100.
* Cutoff Frequency: Range 0–100.
* Q Control: Range 1.0 to 5.0.
* Up/Down: Selects frequency sweep direction.

A-4 Step Phaser
* Rate, Depth, Manual, Resonance: Each range 0–100.
* LFO Step: Range 0–100 (applies changes in a stepped manner).

A-5 Parametric EQ (4-Band):
* High/Low Frequency: 2–8 kHz / 60–250 Hz.
* Middle Frequencies: High Mid (500 Hz–4 kHz), Low Mid (125 Hz–1 kHz).
* Q Controls (Mid bands): 1.0 to 5.0.
* Level (per band): -12 to +12 dB.

A-6 Noise Suppressor:
* Sensitivity, Release, Level: Each range 0–100.

#### Block B: Modulation and Delay Effects

B-1 Short Delay:
* Delay Time (0–100 ms), 
* Effect Level (0–100).

B-2a Chorus:
* Pre Delay (0–100 ms), Rate, Depth, Effect Level (each 0–100).
 
B-2b Flanger:
* Rate, Depth, Manual, Resonance (each 0–100).

B-2c Pitch Shifter:
* Chromatic: -12 to +12 (semitones).
* Fine: -50 to +50.
* Balance: E:D ratio (0–100).
* Feedback, Pre Delay: 0–100 / 0–100 ms.

B-2d Space-D: 
* Mode 1–4.

B-3 Auto Panpot:
* Rate, Depth (each 0–100)
* Mode (Panning/Tremolo).

B-4 Tap Delay (3-way):
* Taps (Center, Left, Right): 0–1200 ms each.
* Levels (C, L, R): 0–100 each.
* Feedback: 0–100.
* Cutoff: 500 Hz to 8 kHz or Thru.

B-5 Reverb:
* Decay (0.5–20 sec)
* Pre Delay (0–100 ms)
* Effect Level (0–100)
* Cutoff (500 Hz–8 kHz)
* Mode (10 types: Room/Hall/Plate/Spring).

B-6 Lineout Filter:
* Presence, Treble, Middle, Bass (each 0–100).
