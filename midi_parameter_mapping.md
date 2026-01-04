# ADDRESS MAPPING OF PARAMETERS

The address is displayed under 7-bit hexadecimal notation.

| Address | MSB | | LSB |  |
|---------|-----|-|-----|--|
| 7bits hex | 0A | Db | Ee |
| Binary | 0000 abbc | 0ddd dddd | 0eee eeee |
| | < Description > | | |
| | a | Non-verifiable / Verifiable | | 0 / 1 | 
| | bb | Bulk Dump Type | | 0 - 3 |
| | | | | ( Number / bank / Group / All ) |
| | c | Temporary Internal Memory | | 0 / 1 |
| | ddd dddd | Patch Number | | (Group-Bank - Number) |
| | | | | 0 ( A - 1 1) |
| | | | |  ... |
| | | | | 7 ( A - 1 8) |
| | | | | 8 ( A - 2 1) |
| | | | |  ... |
| | | | | 63 ( A - 8 8) |
| | | | | 64 ( B - 1 1) |
| | | | |  ... |
| | | | | 127 ( B - 8 8) |
| | eee eeee | Parameter Address | |  |

The actual memory location of each parameter is the Start Address figure of each block plus the Offset Address figure.

## Temporary Area

This is the data area for parameter settings to be monitired and edited. When the "ESCAPE" key is pressed while in patch change or Edit Mode, the parameter settings of the patch currently displayed on the panel will be loaded into this area from the internal memory. According to the MSB data address, this area is divided into non-verifiable and befifiable section, each section containing data with the same parameter values. Normal data transmissions are handled in the non-verifiable area. When Bulk Dump Operations are executed from the front pnalep of the GP-16, data in the verifiable area will be transmitted.

| Start Address | Description |  |
|--------------|-------------|------|
| 00 00 00 | Non verifiable Temorary Data | Table 1 & 2 |
| 08 00 00 | Verifiable Temporary Data | |

## Internal Memory Area
This is the data area for individual patch parameter settings in the internal memory area. According to the MSB data address, this area is divided into non-verifiable and verfiable sectors each sector containg patches with the same parameter values. Normal data transmissions are handled in the non-verifiable area. When Bulk Dump operations are executed from the front panel of the GP-16, the data in the verifiable area will be transmitted.

| Start Address | Description |
|--------------|-------------|
| 01 00 00H | Non-verifiable internal Data (Number) | A-1-1 | Table 1 & 3 |
| 01 ... 00H |  |  |  |
| 01 7F 00H | Non-verifiable internal Data (Number) | B-8-8 |  |
| | | |
| 03 00 00H | Non-verifiable internal Data (Number) | A-1-1 |  |
| 03 ... 00H |  |  |  |
| 03 7F 00H | Non-verifiable internal Data (Number) | B-8-8 |  |
| | | |
| 05 00 00H | Non-verifiable internal Data (Number) | A-1-1 |  |
| 05 ... 00H |  |  |  |
| 05 7F 00H | Non-verifiable internal Data (Number) | B-8-8 |  |
| | | |
| 07 00 00H | Non-verifiable internal Data (Number) | A-1-1 |  |
| 07 ... 00H |  |  |  |
| 07 7F 00H | Non-verifiable internal Data (Number) | B-8-8 |  |
| | | |
| 09 00 00H | Non-verifiable internal Data (Number) | A-1-1 |  |
| 09 ... 00H |  |  |  |
| 09 7F 00H | Non-verifiable internal Data (Number) | B-8-8 |  |
| | | |
| 0D 00 00H | Non-verifiable internal Data (Number) | A-1-1 |  |
| 0D ... 00H |  |  |  |
| 0D 7F 00H | Non-verifiable internal Data (Number) | B-8-8 |  |
| | | |
| 0F 00 00H | Non-verifiable internal Data (Number) | A-1-1 |  |
| 0F ... 00H |  |  |  |
| 0F 7F 00H | Non-verifiable internal Data (Number) | B-8-8 |  |

### Table 1
The binary notation in the left column of the "Description" indicates the formation of each parameter, while the decimal notation at the rigth displays the range of the parameter the settings available. All data sent to the GP-16 effect must be within this range, or the desired effect will not be obtained.
\* When data exceeding this range is included in the internal memory area, such parameters are automatically set to the value of this range when the GP-16 is turned on.

| Offset address | Description | Range | | 
|----------------|-------------|-------|-|
| 00 | 0000 0aaa | JOINT DATA GROUP-A | * 0 - 4 (EFFECT 1-5) |
| 01 | 0000 0aaa | JOINT DATA GROUP-A | * 0 - 4 (EFFECT 1-5) |
| 02 | 0000 0aaa | JOINT DATA GROUP-A | * 0 - 4 (EFFECT 1-5) |
| 03 | 0000 0aaa | JOINT DATA GROUP-A | * 0 - 4 (EFFECT 1-5) |
| 04 | 0000 0aaa | JOINT DATA GROUP-A | * 0 - 4 (EFFECT 1-5) |
| 05 | 0000 0101 | JOINT DATA GROUP-A | 5 (FIXED) |
| 06 | 0000 0aaa | JOINT DATA GROUP-B | ** 6 - 10 (EFFECT 1-5) |
| 07 | 0000 0aaa | JOINT DATA GROUP-B | ** 6 - 10 (EFFECT 1-5) |
| 08 | 0000 0aaa | JOINT DATA GROUP-B | ** 6 - 10 (EFFECT 1-5) |
| 09 | 0000 0aaa | JOINT DATA GROUP-B | ** 6 - 10 (EFFECT 1-5) |
| 0A | 0000 0aaa | JOINT DATA GROUP-B | ** 6 - 10 (EFFECT 1-5) |
| 0B | 0000 0101 | JOINT DATA GROUP-B | 11 (FIXED) |
| | | |
| 0C | 0000 00aa | EFFECT ON/OFF MSB  | 0-3 (chorus/flanger/pitch shifter/space-d) |
|    |           | MODE SELECT - Block B2 | |
| 0D | 0axb cdef | EFFECT ON/OFF      | 0 - 127 |
| 0E | 0ghi jklm | EFFECT ON/OFF LSB  | 0 - 127 |
|    |           | a MODE SELECT - Block A2 | 0 - Distortion, 1 - Overdrive |
|    |           | x Unused           |     |
|    |           | b EFFECT ON/OFF - Block B-6 | 0-1 (OFF/ON) |
|    |           | c EFFECT ON/OFF - Block B-5 | 0-1 (OFF/ON) |
|    |           | d EFFECT ON/OFF - Block B-4 | 0-1 (OFF/ON) |
|    |           | e EFFECT ON/OFF - Block B-3 | 0-1 (OFF/ON) |
|    |           | f EFFECT ON/OFF - Block B-2 | 0-1 (OFF/ON) |
|    |           | g EFFECT ON/OFF - Block B-1 | 0-1 (OFF/ON) |
|    |           | h EFFECT ON/OFF - Block A-6 | 0-1 (OFF/ON) |
|    |           | i EFFECT ON/OFF - Block A-5 | 0-1 (OFF/ON) |
|    |           | j EFFECT ON/OFF - Block A-4 | 0-1 (OFF/ON) |
|    |           | k EFFECT ON/OFF - Block A-3 | 0-1 (OFF/ON) |
|    |           | l EFFECT ON/OFF - Block A-2 | 0-1 (OFF/ON) |
|    |           | m EFFECT ON/OFF - Block A-1 | 0-1 (OFF/ON) |
| | | |
| 0F | 0aaa aaaa | **COMPRESSOR** TONE    | 0-100 (-50 - +50) |
| 10 | 0aaa aaaa | ATTACK             | 0-100 |
| 11 | 0aaa aaaa | SUSTAIN            | 0-100 |
| 12 | 0aaa aaaa | LEVEL              | 0-100 |
| | | |
| 13 | 0aaa aaaa | **DISTORTION** TONE    | 0-100 (-50 - +50) |
| 14 | 0aaa aaaa | DISTORTION         | 0-100 |
| 15 | 0aaa aaaa | LEVEL              | 0-100 |
| | | |
| 16 | 0aaa aaaa | **OVERDRIVE** TONE    | 0-100 (-50 - +50) |
| 17 | 0aaa aaaa | DRIVE              | 0-100 |
| 18 | 0aaa aaaa | TURBO              | 0-1 (ON/OFF) |
| 19 | 0aaa aaaa | LEVEL              | 0-100 |
| | | |
| 1A | 0aaa aaaa | **PICKING FILTER** SENS | 0 - 100 |
| 1B | 0aaa aaaa | CUTOFF FREQ        | 0 - 100 |
| 1C | 0aaa aaaa | Q                  | 0 - 40 (1.0 - 5.0) |
| 1D | 0000 000a | UP/DOWN            | 0-1 (UP/DOWN) |
| | | |
| 1E | 0aaa aaaa | **STEP PHASER** RATE | 0 - 100 |
| 1F | 0aaa aaaa | DEPTH              | 0 - 100 |
| 20 | 0aaa aaaa | MANUAL             | 0 - 100 |
| 21 | 0aaa aaaa | RESONANCE          | 0 - 100 |
| 22 | 0aaa aaaa | LFO STEP           | 0 - 100 |
| | | |
| 23 | 0aaa aaaa | **PARAMETRIC EQ** HI FREQ | 0 - 100 (2kHz - 8kH2) |
| 24 | 00aa aaaa | HI LEVEL           | 0 - 48 (-12 - +12dB) |
| 25 | 0aaa aaaa | H.M FREQ           | 0 - 100 (500 - 4kH2) |
| 26 | 0aaa aaaa | H.MID Q            | 0 - 40 (1.0 - 5.0) |
| 27 | 00aa aaaa | H.M LEV            | 0 - 48(-12 - +12dB() |
| 28 | 0aaa aaaa | L.M FHEQ           | 0 - 100(125 - 1kH2) |
| 29 | 0aaa aaaa | L.MID D            | 0 - 40 (1.0 - 5.0) |
| 2A | 00aa aaaa | L.M LEV            | 0 - 48 (-12 - +12dB) |
| 2B | 0aaa aaaa | LO FREQ            | 0 - 100 (40 - 250Hz) |
| 2C | 00aa aaaa | LO LEVEL           | 0 - 48 (-12 - +12dB) |
| 2D | 00aa aaaa | OUT LEV            | 0 - 48(-12 - +12dB) |
| | | |
| 2E | 0aaa aaaa | **NOISE SUPPRESSOR** SENS | 0 - 100 |
| 2F | 0aaa aaaa | RELEASE            | 0 - 100 |
| 30 | 0aaa aaaa | LEVEL              | 0 - 100 |
| | | |
| 31 | 0aaa aaaa | **SHORT DELAY**  D.TIME | 0 - 100 |
| 32 | 0aaa aaaa | E.LEVEL            | 0 - 100 |
| | | |
| 33 | 0aaa aaaa | **CHORUS** P.DELAY | 0 - 100 |
| 34 | 0aaa aaaa | RATE               | 0 - 100 |
| 35 | 0aaa aaaa | DEPTH              | 0 - 100 |
| 36 | 0aaa aaaa | E.LEVEL            | 0 - 100 |
| | | |
| 37 | 0aaa aaaa | **FLANGER** RATE   | 0 - 100 |
| 38 | 0aaa aaaa | DEPTH              | 0 - 100 |
| 39 | 0aaa aaaa | MANUAL             | 0 - 100 |
| 3A | 0aaa aaaa | RESONANCE          | 0 - 100 |
| | | |
| 3B | 0000 000a | **PITCH SHIFTER** BAL MSB | 0 - 200 |
| 3C | 0aaa aaaa | BAL LSB            | (E:0 D:100 - E:100 D:0) |
| 3D | 000a aaaa | CHROMATIC          | 0 - 24 (-12 - +12) |
| 3E | 0aaa aaaa | FINE               | 0 - 100 (-50 - +50) |
| 3F | 0aaa aaaa | F.BACK             | 0 - 100 |
| 40 | 0aaa aaaa | P. DELAY           | 0 - 100 (0 - 100ms) |
| | | |
| 41 | 0000 00aa | **SPACE D** MODE   | 0-3 (1-4) |
| 42 | 0aaa aaaa | **AUTO PANPOT** RATE | 0 - 100 |
| 43 | 0aaa aaaa | DEPTH              | 0 - 100 |
| 44 | 0000 000a | MODE               | 0 - 1 (PANPOT/TREMOLO)
| | | |
| 45 | 0000 aaaa | **TAP DELAY** C-TAP MSB | 0 - 1200  |
| 46 | 0aaa aaaa | C-TAP LSB          | (0 - 1200ms) |
| 47 | 0000 aaaa | L-TAP MSB         | 0 - 1200  |
| 48 | 0aaa aaaa | L-TAP LSB         | (0 - 1200ms) |
| 49 | 0000 aaaa | R-TAP MSB         | 0 - 1200 | 
| 4A | 0aaa aaaa | R-TAP LSB         | (0 - 1200ms) |
| 4B | 0aaa aaaa | C.LEVEL           | 0 - 100 |
| 4C | 0aaa aaaa | L.LEVEL           | 0 - 100 |
| 4D | 0aaa aaaa | R.LEVEL           | 0 - 100 |
| 4E | 0aaa aaaa | F.NACK            | 0 - 100 |
| 4F | 0000 000a | CUTOFF MSB        | 0 - 200 |
| 50 | 0aaa aaaa | CUTOFF LSB        | (500Hz - 8kHz THRU) |
| | | |
| 51 | 0000 aaaa | **REVERB** DECAY  | 0 - 45 (0.5 - 5ms)<br/>46 - 75 (5.5 - 20ms) |
| 52 | 0000 aaaa | MODE              | 0 - 9<br/>(ROOM1/ROOM2/ROOM3<br/>HALL1/HALL2/HALL3<br/>PLATE1/PLATE2/SPRING1<br/>SPRING2) |
| 53 | 0000 000a | CUTOFF MSB        | 0 - 200 |
| 54 | 0aaa aaaa | CUTOFF LSB        | (500Hz-8kHz, THRU) |
| 55 | 0aaa aaaa | P.DELAY           | 0 - 100 (0-100ms) |
| 56 | 0aaa aaaa | E.LEVEL           | 0 - 100 |
| | | |
| 57 | 0aaa aaaa | **LINEOUT FILTER** PRESENCE | 0 - 100 |
| 58 | 0aaa aaaa | TREBLE            | 0 - 100 |
| 59 | 0aaa aaaa | LEVEL             | 0 - 100 |
| 5A | 0aaa aaaa | BASS              | 0 - 100 |
| | | |
| 5B | 0aaa aaaa | **MASTER VOLUME** | 0 - 100 |
| | | |
| 5C | 0aaa aaaa | **EXPRESSION ASSIGN** | 0 - 70, 127 (Table 4) |
| 5D | 0000 000a | EXPRESSION DEVICE | 0-1 (PEDAL/LFO) |
| 5E | 0aaa aaaa | LFO RATE          | 0 - 100 |
| 5F | 0000 aaaa | EXPRESSION MAX LEVEL MSB         | 0 - 1200 | 
| 60 | 0aaa aaaa | EXPRESSION MAX LEVEL LSB    | (depends on expression assign) |
| 61 | 0000 aaaa | EXPRESSION MIN LEVEL MSB         | 0 - 1200 | 
| 62 | 0aaa aaaa | EXPRESSION MIN LEVEL LSB    | (depends on expression assign) |
| | | |
| 63 | 0000 00aa | **OUTPUT CHANNEL** | 0-2 (channel 1, channel 2, channel 1&2)
| | | |
| 64 | 0aaa aaaa | **PATCH NAME 0** |  |
| ... | ... | ... | | 32-127 ASCII CODE |
| 73 | 0aaa aaaa | PATCH NAME 15 |  |
| 74 | 0aaa aaaa | END OF PATCH NAME | 0 (fixed) |

\*,\*\*, .... JOINT DATA parameters, which define the order of the 12 effects should consist of figures compromising 0 to 4 for Address 00 - 0A, without being mistakenly repeated.
**EXAMPLE**
With the GP-16 assigned to MIDI Receiv Chanel No. 1, transmit thef following message to the GP-16 in order to recall the OUTPUT CNAHHEL DATA of patch B1-1 from the internal memory area data
<code>F0 41 00 2a 10 01 40 63 00 00 01 5B F7</code>

### Table 2

| Offset<br/>Address | Range | Description |
| -------------------|-------|-------------|
| 75 | 0aaa aaaa | SOUND CHANGE REQUEST for Temporary Area 0-127 (FREE) |
| 76 | 0000 0000 | |
| ...| ... | Dummy ignored if received |
| 7F | 0000 0000 | |

\* The SOUND CHANGE REQUEST command receieved by the GP-16 is handled only in the temporary data area.
Hwne the GP-16 receis any data and this command at the end from an exernal MIDI vecies, the GP-16 will chage its' sound. If it is impossible to receive the SOUND CHANGE REUQEST command the sound will change whent the GP-16 is manualy switched into Edit Mode. The SOUND CHANGE REQUEST command is included in the bulk dump data.

**EXAMPLE**
When the GP-16 is set to MIDI Reecieve Channel No 1 in the Play Mode, transmitting two messages to the GP-16 activates the GP-16 effect bypass function, turning all effects off.

<code>
F0 41 00 2A 12 00 00 0D 00 00 73 F7
</code>
<br/>
<code>
 F0 41 00 2A 12 00 00 75 00 00 F7
</code>

### Table 3
| Offset<br/>Address | Range | Description |
| -------------------|-------|-------------|
| 75 | 0000 0000 | |
| ...| ... | Dummy - ignored if received |
| 7F | 0000 0000 | |

### Table 4
| Offset | Description | Nr | Parameters |
|--------|-------------|----|------------|
| 5C| 0000 0000 | 0  | **COMPRESSOR**  TONE |
|   | 0000 0001 | 1  | ATTACK |
|   | 0000 0010 | 2  | SUSTART |
|   | 0000 0011 | 3  | LEVEL |
|   |           |    |       |
|   | 0000 0100 | 4  | **DISTORTION** TONE |
|   | 0000 0101 | 5  | DISTORTION |
|   | 0000 0110 | 6  | LEVEL |
|   | 0000 0111 | 7  | **OVERDRIVE** TONE |
|   | 0000 1000 | 8  | DRIVE |
|   | 0000 1001 | 9  | TURBO |
|   | 0000 1010 | 10 | LEVEL |
|   |           |    |       |
|   | 0000 1011 | 11 | **PICKING FILTER** SENS |
|   | 0000 1100 | 12 | CUTOFF FREQ |
|   | 0000 1101 | 13 | Q |
|   | 0000 1110 | 14 | UP/DOWN |
|   |           |    |       |
|   | 0000 1111 | 15 | **STEP PHANSER** RATE |
|   | 0001 0000 | 16 | DEPTH |
|   | 0001 0001 | 17 | MANLAL |
|   | 0001 0010 | 18 | RESONANCE |
|   | 0001 0011 | 19 | LFO STEP |
|   |           |    |       |
|   | 0001 0100 | 20 | **PARAMETRIC EQ** HI FREQ |
|   | 0001 0101 | 21 | HI LEVEL |
|   | 0001 0110 | 22 | H.M FREQ |
|   | 0001 0111 | 23 | H.MID Q |
|   | 0001 1000 | 24 | H.M LEV |
|   | 0001 1001 | 25 | L.M FREQ |
|   | 0001 1010 | 26 | L.MID Q |
|   | 0001 1011 | 27 | L.M LEV |
|   | 0001 1100 | 28 | LO FREQ |
|   | 0001 1101 | 29 | LO LEVEL |
|   | 0001 1110 | 30 | OUT LEV |
|   |           |    |       |
|   | 0001 1111 | 31 | **NOISE SUPPRESSION** SENS |
|   | 0010 0000 | 32 | RELEASE |
|   | 0010 0001 | 33 | LEVEL |
|   |           |    |       |
|   | 0010 0010 | 34 | **SHORT DELAY** D. TIME |
|   | 0010 0011 | 35 | E. LEVEL |
|   |           |    |       |
|   | 0010 0100 | 36 | **CHORUS** P. DELAY |
|   | 0010 0101 | 37 | RATE |
|   | 0010 0110 | 38 | DEPTH |
|   | 0010 0111 | 39 | E. LEVEL |
|   |           |    |       |
|   | 0010 1000 | 40 | **FLANGER** RATE |
|   | 0010 1001 | 41 | DEPTH |
|   | 0010 1010 | 42 | MANUAL |
|   | 0010 1011 | 43 | RESONANCE |
|   |           |    |       |
|   | 0010 1100 | 44 | **PITCH SHIFTER** BALANCE |
|   | 0010 1101 | 45 | CHROMATIC |
|   | 0010 1110 | 46 | FINE |
|   | 0010 1111 | 47 | F. BACK |
|   | 0011 0000 | 48 | P. DELAY |
|   |           |    |       |
|   | 0011 0001 | 49 | **SPACE D** MODE |
|   |           |    |       |
|   | 0011 0010 | 50 | **AUTO PANPOT** RATE |
|   | 0011 0011 | 51 | DEPTH |
|   | 0011 0100 | 52 | MODE |
|   |           |    |       |
|   | 0011 0101 | 53 | **TAP DELAY** C. TAP |
|   | 0011 0110 | 54 | L. TAP |
|   | 0011 0111 | 55 | R. TAP |
|   | 0011 1000 | 56 | C. LEVEL |
|   | 0011 1001 | 57 | L. LEVEL |
|   | 0011 1010 | 58 | R. LEVEL |
|   | 0011 1011 | 59 | F. BACK |
|   | 0011 1100 | 60 | CUTOFF |
|   |           |    |       |
|   | 0011 1101 | 61 | **REVERB** DECAY |
|   | 0011 1110 | 62 | MODE |
|   | 0011 1111 | 63 | CUTOFF |
|   | 0100 0000 | 64 | P. DELAY |
|   | 0100 0001 | 65 | E. LEVEL |
|   |           |    |       |
|   | 0100 0010 | 66 | **LINEOUT FILTER** PRESENCE |
|   | 0100 0011 | 67 | TREBLE |
|   | 0100 0100 | 68 | MIDDLE |
|   | 0100 0101 | 69 | BASS |
|   |           |    |       |
|   | 0100 0110 | 70 | **MASTER VOLUME** |
|   |           |    |       |
|   | 0111 1111 | 127 | **EXPRESSION ASSIGN** OFF |

**Address Map**

| Address  |  Block             | Sub Block | Reference           |
|----------|--------------------|-----------|---------------------|
| 00-00-00 | Non<br/>verifiable |           | Table 1<br/>Table 2 |
| 00-01-00 | Area               |           | Table 1<br/>Table 3 |
| 01-00-00 |                    |           |                     |
| 02-00-00 |                    |           |                     |
| 03-00-00 |                    |           |                     |
| 04-00-00 |                    |           |                     |
| 05-00-00 |                    |           |                     |
| 06-00-00 |                    |           |                     |
| 07-00-00 |                    |           |                     |
| 08-00-00 | Verifiable         |           | Table 1<br/>Table 2 |
| 08-01-00 | Area               |           | Table 1<br/>Table 3 |
| 09-00-00 |                    |           |                     |
| 0A-00-00 |                    |           |                     |
| 0B-00-00 |                    |           |                     |
| 0C-00-00 |                    |           |                     |
| 0D-00-00 |                    |           |                     |
| 0E-00-00 |                    |           |                     |
| 0F-00-00 |                    |           |                     |

## Parameter Memory Areas

The actual effective location of each parameter is the Base Address figure of each block plus the Offset Address figure.

### Editable Area for Parameter Settings

This is the data area for parameters settings to be memorized and edited. When the TRANSMIT is changed, value in the RCV DT becomes the present value in the TMP area (if receive function is enabled) or DT is TMP (patch current). Displayed on the panel will be AMHIED into this area from the transmit section is a executed from the TMP area(1) of the DISC before the actual information of memory changes is executed from the TMP area(1) of the DISC before the actual content will not be changed.
