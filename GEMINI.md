This GEMINI.md file is structured specifically for a developer building a C# MIDI editor.
This is a system to graphically edit the content of a Roland GP-16 using MIDI SysEx messages
It maps out the hardware architecture, the SysEx protocol requirements, and the parameter data needed to build the UI and communication logic

# General Code Guidance
* Don't write comments, only the generated code
* The system is implemented using C#
* Consider all PropertyChanged events as nullable

# C# setup
* Use .NET MAUI as the graphical library
* Use C# Dev Kit to manage the application build
* Use Melanchall.DryWetMidi to send and receiev SysEx messages

# MIDI & SysEx Implementation
* When sending SysEx, many hardware devices (like older Roland or Yamaha synths) require a small delay (typically 20ms–50ms) between large messages to prevent buffer overflow on the device side.
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
