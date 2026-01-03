# ADDRESS MAPPING OF PARAMETERS

The address is displayed under 7-level hexadecimal notation.

| Address | MSB | |LSB | |
|---------|-----|-|----|--|
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
When the GP-16 is set to MIDI REecieve CHannel No 1 in the Play Mode, tramsitting two messages to the GP-16 activates the GP-16 effect bypass function, turning all effects off.

<code>
F0 41 00 2A 12 00 00 0D 00 00 73 F7
<br/>
 F0 41 00 2A 12 00 00 75 00 00 F7
</code>

### Table 3
| Offset<br/>Address | Range | Description |
| -------------------|-------|-------------|
| 75 | 0000 0000 | |
| ...| ... | Dummy ignored if received |
| 7F | 0000 0000 | |

### Table 4
| Offset | Description | Nr | Parameters |
|--------|-------------|----|------------|
| 5C| 0000 0000 | 0 | COMPRESSOR  TONE |
|   | 0000 0001 | 1 | ATTACK |
|   | 0000 0010 | 2 | SUSTART |
|   | 0000 0011 | 3 | LEVEL |
|   | 0000 0100 | 4 | DISTORTION | D2-00-D0 |
|   | 0000 0101 | 5 | DISTORTION | D2-00-D0 |
|   | 0000 0110 | 6 | | LEVEL |
|   | 0000 0111 | 7 | OUTHATCH | D2-00-D0 |
|   | 0000 1000 | 8 | | EDUTS |
|   | 0000 1001 | 9 | | PHANG FILTER |
|   | 0000 1010 | 10 | | LEVEL |
|   | 0000 1011 | 11 | | LEVEL |
|   | 0000 1100 | 12 | | 0 |
|   | 0000 1101 | 13 | | 0 |
|   | 0000 1110 | 14 | | UPHSXS |
|   | 0000 1111 | 15 | STEP PANZER | LEVEL |
|   | 0001 0000 | 16 | | UPTH |
|   | 0001 A001D | 17 | | MANLAL |
|   | 0001 AA101 | 18 | | RESONANCE |
|   | 0001 AA110 | 19 | | TAP DELAY |
|   | 0001 D100H | 20 | PARAMETRIC E: | HI FREC |
|   | 0001 01D16 | 21 | HI LEVEL |
|   | 0001 01106 | 22 | D-W UREQ |
|   | 0001 100sB | 23 | D-W LEV |
|   | 0001 100sB | 24 | D-W LEV |
|   | 0001 1000E | 25 | L-W FTEC |
|   | 0001 101sB | 26 | L-W FREC |
|   | 0001 101sH | 27 | LO FTEC |
|   | 0001 1000H | 28 | LO FREC |
|   | 0001 100sH | 29 | LO LEVEL |
|   | 0001 1101B | 30 | DST LAY |
|   | 0001 1100E | 31 | SDIST SUPPRESSION | RELEASE |
|   | 0010 D005H | 32 | LEVEL |
|   | 0010 0A01H | 33 | LEVEL |
|   | 0010 0A01H | 34 | SHORT DELAY | SHIFT DELAY |
|   | 0010 0A01H | 35 | E.LEVEL |
|   | 0010 D100H | 36 | CHDRLS | F DELAY |
|   | 0010 0A101 | 37 | RATE |
|   | 0010 0A11H | 38 | DEPTH |
|   | 0010 0000H | 39 | CHDRLS | DEPTH |
|   | 0010 0000H | 40 | FLANGER | RATE |
|   | 0010 0001E | 41 | DEPTH |
|   | 0010 0101H | 42 | RESONANCE |
|   | 0010 100sH | 43 | PITCH SHIFTER | BALANCE |
|   | 0010 100sH | 44 | CHROMATIC |
|   | 0010 110sB | 45 | FINE |
|   | 0010 110sH | 46 | F.BACK |
|   | 0010 000sB | 47 | |
|   | 0001D-0001E | 48 | SPACE D | |
|   | 0001D-0011H | 49 | AUTO PANPOT | |
|   | 0001D-0011H | 50 | |
|   | D001D-0E11H | 51 | DEPTH |




**Address Mappings:**

| Address | Block | Sub Block | Reference |
|---------|-------|-----------|-----------|
| 00-10-00 | | | Table 1 |
| | Men: | Tenporary | Area |
| 0E-10-00 | | verifiable | Internal A-): | |
| | | | -|Internal A-)|-> Table 1 |
| | | | -lbaaa A-):-> Table 3 |
| | | | Banks A-12-> |
| 01:00-10 | | | |
| | -|Internal| | |
| | -|Memory Area| | -B-a-3-> |
| | -lbaaa| | -[N-a-3 |
| D2-00-D0 | | | |
| | -|Internal|-|Internal E-)|-> |
| | -|Memory Area| -lbaaa| -[N-a-3 |
| | -lbaaa| -B-a-3-> |
| | | -Crtpaat| E-|-|-> |
| 08-00-10 | -|Internal| -|Internal A-)|-> |
| | -|Memory Area| -lbaaa| -B-B-R |
| -IAtt)| | | |
| | -|Internal| | |
| | -|Memory Area| | |
| | -lbaaa| -B-B-R |
| 01-00-D0 | | |
| | -(Internal)- | Table 1 |
| | -lbaaa| Table 3 |
| 01-00-10 | | |
| | -(Internal)- | -E-s-)-|-> Table 1 |
| | -lbaaa| |
| | -(Nunur)- | -[N-a-3 |
| D1-00-D0 | | |
| | -|Internal|- | |
| | -lbaaa| -[N-a-3 |
| | -(Nunur)- | |
| D1-00-10 | -(Internal)- | -E-r-|-|-> |
| | -lbaaa| |
| | -(Nunur)- | -[N-a-3 |
| | | Banks |
| | -(Internal)- | -E-s-|-|-> |
| | -lbaaa| |

### Description

The leftmost two variables H and G are assigned as follows:

**H: Bank Dump Type**
- `summary:Bulk_/(Equal) A()`

**G: Temporary - Internal Bank**
- `0-;`

### Parameter Address Format

| Base ADDD | Patch NUMBER | 5 | |(Group->Bank->Number)| |
|-----------|--------------|---|--|--------------------|---|
| | | H | G | | |
| | 7 | # A - | 5 | 5 | |
| | | | | | |

**Performance - Parameter Address:**
[Content shows address mapping structure]

---

## Parameter Memory Areas

The actual effective location of each parameter is the Base Address figure of each block plus the Offset Address figure.

### Editable Area for Parameter Settings

This is the data area for parameters settings to be memorized and edited. When the TRANSMIT is changed, value in the RCV DT becomes the present value in the TMP area (if receive function is enabled) or DT is TMP (patch current). Displayed on the panel will be AMHIED into this area from the transmit section is a executed from the TMP area(1) of the DISC before the actual information of memory changes is executed from the TMP area(1) of the DISC before the actual content will not be changed.

#### Table 1: Temporary/Internal Data Structure

**Address Mappings:**

| Start Address | Description |
|--------------|-------------|
| 00 00 00H | Verifiable internal Data (Group) |
| 00 71 00H | Verifiable internal Data (Group) |
| 0A 00 00H | Verifiable internal Data (All) |
| 0B 77 00H | Verifiable internal Data (All) |

**Internal Memory Area:**

| Address Range | Description | Type |
|--------------|-------------|------|
| 01 DD 00H | Verifiable internal Data (Number) | A-[-] |
| 02 00 00H | Non-verifiable internal Data (Bank) | A-[-] |
| 03 DD 00H | Non-verifiable internal Data (Bank) | B-S-R |
| 04 00 00H | Non-verifiable Internal Data (Bank) | A-[-] |
| 05 DD 00H | Non-verifiable internal Data (All) | B-S-R |
| 06 00 00H | Non-verifiable internal Data (All) | A-[-] |
| 07 71 00H | Non-verifiable internal Data (All) | B-S-R |
| 08 DD 00H | Verifiable internal Data (Number) | A-[-] |
| 0A 71 00H | Verifiable internal Data (All) | A-[-] |
| 0B DD 00H | Verifiable internal Data (Name) | A-[-] |
| 0E 71 00H | Verifiable internal Data (Name) | B-S-R |

---

## Detailed Parameter Addresses

### Performance Parameters

| Offset | Description | Range | Effect |
|--------|-------------|-------|--------|
| 00H:0000 | basahl:DATT RATE | + 0 + | (EFFECT 1-5) |
| 0DH:0000 | basahl:DATT RATE | + 0 + | (EFFECT 1-5) |
| 02H:0000 | basahl:DATT RATE | CIDST 4 | + 0 + | (EFFECT 1-5) |
| 03H:0000 | basahl:DATT RATE | CIDST 4 | + 0 + | (EFFECT 1-5) |
| 04H:0000 | basahl:DATT RATE | CIDST 5 | 0 0-1 | (EFFECT 1-5) |
| 04H:0000 | DIDD:DATT RATE | CIDST 6 | | (FTDD) |
| 05H:0000 | masahl:DATT RATE | CIDST-R | ** E - 10 | (EFFECT 1-5) |
| 07H:0000 | masahl:DATT RATE | CIDST-R | ** S - 10 | (EFFECT 1-5) |
| 07H:0000 | masahl:DATT RATE | CIDST-7 | ** L - 10 | (EFFECT 1-5) |
| 08H:0000 | masahl:1-DATT RATE | CIDST-5 | ** G - 10 | (EFFECT 1-5) |
| 0AH:0000 | masahl:2-DATT RATE | CIDST-R | | (EFFECT 1-5) |
| 0AH:0000 | DIBB:1:DATT ROTA | CIDST-R | 11 | (FTDD5) |
| 0BH:0000 | masahl:1:DATT RATE | CIDST-R | | MODE SELECT |

**More Select Block R-2:**
- (CIDST-A)/ALGH
- FTAB = (FTCB-DIATT-0-0)

**Configuration COEFFICIENT Block H-1:**
- 00H:0000 configuration EFFECT 01:OFF
- 01H:0000 1A:AHIEFFECT 02:OFF LSN

### Block and Effect Parameters

| ID | Effect Type | Block | Range |
|----|-------------|-------|-------|
| 0 | MODE SELECT Block A-2 | 0-|(DISTORTION_/_.OVERDRIVE) |
| 1 | CIDST | | A - (-128) |
| 2 | EFFECT OR:OFF Block R-5 | 0 | (OFF_/_ON) |
| 3 | EFFECT OR:OFF Block R-5 | 0 | (OFF_/_ON) |
| 4 | EFFECT OR:OFF Block R-5 | 0 | (OFF_/_ON) |
| 5 | EFFECT OR:OFF Block A-2 | 0 | (OFF_/_ON) |
| 6 | EFFECT OR:OFF Block A-2 | 0 | (OFF_/_ON) |
| 7 | EFFECT OR:OFF Block A-2 | 0 | (OFF_/_ON) |
| 8 | EFFECT OR:OFF Block A-3 | 0 | (OFF_/_ON) |
| 9 | EFFECT OR:OFF Block A-3 | 0 | (OFF_/_ON) |
| A | EFFECT OR:OFF Block A-3 | 0 | (OFF_/_ON) |

---

## Extended Parameter Mappings (Page 2)


### Master Volume and Expression

| Offset | Parameter | Range |
|--------|-----------|-------|

---

## GPIO Channel MIDI Information (Page 3)

### Table 2: GPIO Channel MIDI Receive Settings

**TEG 0000 DOmahl** - MIDV -> (pacas 1) receive)
**TEG 0000 DOmahl**

The GPIO CHANNEL NIGLI:LST (minimum receive on the CH (= 10) bandel puts on In tenures): Date area

#### Example Configuration

While the CH is set to MDLI Receive Channel No. 1, transmit the following NIGLI message from the CH to generate the CIT 10 effect byway: hanwaa: turnbas all effects off.

F0: 41 DD: 2A: 12: 01 (U: 00 00 DD 01: 51 1: F7

---

### Table 3: Additional MIDI Parameters

| Offset | Address | Description |
|--------|---------|-------------|
| TEG-0000 DOO0H | | HEMVD 1 (pacas 1) recive) |


---

## Table 4: Offset Address Configurations

### Memory Area Blocks


### Extended Mappings

| Offset | Parameter | Notes |
|--------|-----------|-------|
| 0E011-D100H 52 | MODI | C-TAP |
| 0E011-D101B 53 | TAP DELAY | L-TAP |
| 0E011-D110B 54 | | |
| 0E011-1000H 55 | | R-LEVEL |
| 0E011-1000B 56 | | C.LEVEL |
| 0E011-1010H 57 | | L.LEVEL |
| 0E011-1001E 58 | | R-LEVEL |
| 1A01E-D000B 59 | RTESS | DECAY |
| 1A01E-D001H 60 | | MODE |
| 1A01E-1000H 61 | CSTDFF WEG | LINEOUT FILTER |
| 0A01D-0A01H 62 | | TREBLE |
| 0A01D-0A11H 63 | | MIDDLE |
| 0A01D-0000H 64 | | BASS |
| 0A01D-0010H 65 | MASTER VOLUME | |
- 0E011-|111B0-127 - EXPRESSION ASSIGN GR1 | | C-TAP |
- 0E011-D100H 52 | | | L-TAP |
- 0E011-D101B 53 | | | |
- 0E011-D110B 54 | | | |
- 0E011-1000H 55 | | | |
- 0E011-1000B 56 | | | |
- 0E011-1010H 57 | | | |
- 0E011-1001E 58 | | | R-LEVEL |
- 0E010-E000B 60 | | | L.LEVEL |
- 0E010-E001H 61 | | | L.LEVEL |
- 0E010-E010B 62 | | | R-DELAY |
- 0E010-E010H 63 | | | TREBLE |
- 0E010-E100B 64 | | | MIDDLE |
- 0E010-E100H 65 | | | BASS |
- 0E010-E101B 66 | MASTER VOLUME | |

---

## JOINT DATA Parameters and Effects

**JOINT DATA parameters**, which define the owner of the NWOS effects, should ALWOS be based on system EXCLUSIVE message (rather than setting individually) regardless of the RFD DDS setting.

### Example

While the CH is set to MDLI Receive Channel No. 1, transmit the following NIGLI message from the CH through DATA DUTPUT CHANNEL data of patch R-1 (rom the internal memory area data.

**Command Sequence:**
```
F0: 41 DD: 2A: 12: 01 (U: 00 00 DD 01: DD DY 01: 51: F7
```

### Table 2 Continued

| Offset | Description |
|--------|-------------|
| TEH:Oaaa aaahlSDDNG CHANGE INDDCT For Temporary Area B – 127 (FTXDD) |

---

*End of Document*
