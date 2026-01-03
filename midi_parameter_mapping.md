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
* When data exceeding this range is included in the internal memory area, such parameters are automatically set to the value of this range when the GP-16 is turned on.

| Offset address | Description | Range | | 
|----------------|-------------|-------|-|
| 00 | 0000 0aaa | JOINT DATA GROUP-A | 0 - 4 (EFFECT 1-5) |
| 01 | 0000 0aaa | JOINT DATA GROUP-A | 0 - 4 (EFFECT 1-5) |
| 02 | 0000 0aaa | JOINT DATA GROUP-A | 0 - 4 (EFFECT 1-5) |
| 03 | 0000 0aaa | JOINT DATA GROUP-A | 0 - 4 (EFFECT 1-5) |
| 04 | 0000 0aaa | JOINT DATA GROUP-A | 0 - 4 (EFFECT 1-5) |
| 05 | 0000 0101 | JOINT DATA GROUP-A | 5 (FIXED) |
| 06 | 0000 0aaa | JOINT DATA GROUP-B | 6 - 10 (EFFECT 1-5) |
| 07 | 0000 0aaa | JOINT DATA GROUP-B | 6 - 10 (EFFECT 1-5) |
| 08 | 0000 0aaa | JOINT DATA GROUP-B | 6 - 10 (EFFECT 1-5) |
| 09 | 0000 0aaa | JOINT DATA GROUP-B | 6 - 10 (EFFECT 1-5) |
| 0A | 0000 0aaa | JOINT DATA GROUP-B | 6 - 10 (EFFECT 1-5) |
| 0B | 0000 0101 | JOINT DATA GROUP-B | 11 (FIXED) |
| 0C | 0000 00aa | EFFECT ON/OFF MSB | 0-3 (chorus/flanger/pitch shifter/space-d)


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

**Effects Mapping:**
- 0FH:0000 masahl:CHRT:LSN | OFT:0000 | 0 - 100 (:+0 - +50:) |
- 10H:0000 masahl | ATTACH | 0 - 100 |
- 11H:0000 masahl | SUSTART | 0 - 100 |
- 12H:0000 masahl | LEVEL | 0 - 100 |
- 13H:0000 masahl:DISTURTION | TONE | 0 - 100 (:+0 - +50:) |
- 14H:0000 masahl | DISTORTION | 0 - 100 |
- 15H:0000 masahl | LEVEL | 0 - 100 |
- 16H:0000 masahl:DISTORTION:C | TONE | 0 - 100 (:+0 - +50:) |
- 17H:0000 masahl | LEVEL | 0 - 100 |
- 18H:0000 DOmahl | TEMPO | 0 - | (OFF_/_ON) |

---

## Extended Parameter Mappings (Page 2)

### Additional Parameters

| Offset | Name | Type | Range |
|--------|------|------|-------|
| 19H | Other masahl | LEVEL | 0 - 100 |
| 1AH | Omaa masahl PTERING FILTER | CUTS | 0 - 100 |
| 1BH | doaa masahl | RESONANCE | 0 - 100 |
| 1CH | [bass masahl] | D | 0 - 40 ((1.0 - 5.0)) |
| 1DH | DOOD DOmahl | | ((0 -_/DAN)) |
| 1EH | Emax masahl STEP PANZER | CUTS | 0 - 100 (:+0 - -50:) |
| 1FH | 'bass masahl | DEPTH | 0 - 100 |
| 20H | 'bass masahl | MANLAL | 0 - 100 |
| 21H | (bass masahl) | RESONANCE | 0 - 100 |
| 22H | (bass masahl) | TAP DELAY | 0 - 100 |
| 23H | 'Omaa masahl:PARAMETRIC E: | ST FREC | 0 - 100 (21 - 8kH2) |
| 24H | Obaa masahl | HI LEVEL | 0 - 48(-12 - -12dB) |
| 25H | taaas masahl | D-W PREC | 0 - 100(100 - 4kH2) |
| 26H | taaas masahl | D-MLD D | 0 - 40 ((1.0 - 5.0)) |
| 27H | Atbaa aHaahl | D-W LEV | 0 - 48(-12 - -12dB() |
| 28H | 'Obaa masahl | D-W FHEQ | 0 - 100(125 - 1kH2) |
| 29H | 'Oaas masahl | L-MLD D | 0 - 40 ((1.0 - 5.0)) |
| 2AH | 02aa masahl | L-M LEV | 0 - 48(-12 - -12dB() |
| 2BH | Agaa masahl | LO FHEQ | 0 - 100(40 - 250H2) |
| 2CH | 'Obaa masahl | LO LEVEL | 0 - 48(-12 - -12dB() |
| 2DH | 'Omaa masahl | DST LAY | 0 - 48(-12 - -12dB() |

### Master Volume and Expression

| Offset | Parameter | Range |
|--------|-----------|-------|
| 2EH | 0aaa masahl SDIST SUPPRESSION | 0 - 100 |
| 2FH | 'Onaa masahl | RELEASE | 0 - 100 |
| 30H | 'Oaas masahl | LEVEL | 0 - 100 |
| 31H | 'Oaaa masahl SHIFT DELAY | D-TIME | 0 - 100 |
| 32H | 'Oaaa masahl | LEVEL | 0 - 100 |
| 33H | 'Onaa masahl CHDRLS | F DELAY | 0 - 100 |
| 34H | 'Oaaa masahl | RATE | 0 - 100 |
| 35H | 'Oaaa masahl | DEPTH | 0 - 100 |
| 36H | 'Oaas masahl | E.LEVEL | 0 - 100 |
| 37H | 'Oaaa masahl :-FLANGER | DEPTH | 0 - 100 |
| 38H | baa masahl | DEPTH | 0 - 100 |
| 39H | 'Oaas masahl | TIME | 0 - 100 |
| 3AH | 'Oaaa masahl | RESONANCE | 0 - 100 |
| 3BH | 1000 DOFTCH SHIFTER | BAL WEG | 0 - 200 |
| 3CH | aaa masahl | BAL WEG | 0-(D-100 (-100-100)) |
| 3DH | Obbbb masahl | CHROMTIC | 0 - 24 (:-12 - -12:) |
| 3EH | 'Oaaa masahl | FINE | 0 - 100 (:-50 - -50:) |
| 3FH | (baaa masahl) | F.BACK | 0 - 100 |
| 40H | 0aaa aHaahl | F DELAY | 0 - 120 (0 - 100ms) |
| 41H-42H | 0000 DsaahlSPACE D | MODE | 0-3(-|-(2-4)) |
| 43H | 'Oaaa aaahl AUTO PANPOT | RATE | 0 - 100 |
| 42H | 'baaa masahl | DEPTH | 0 - 100 |

### Additional Mapping Parameters

| Address | Parameter | Type | Range |
|---------|-----------|------|-------|
| 43H-000D DOmahl | MODI | 0 - | |
| 43H-4000 masahl TAP DELAY | C-TAP WEG | 0 - 1200 | |(10ms)| |
| 46H doaa masahl | C-TAP FHEQ | 0 - 200 | |
| 47H doaa masahl | L-TAP WEG | 0 - 1200 | |(10ms)| |
| 48H doaa masahl | L-TAP LAG | 0 - | |(10ms)| |
| 49H-040D masahl | R-TAP WEG | 0 - 1200 | |(20ms)| |
| 4AH (baaa masahl) | R-TAP LAG | 0 - | |
| 4BH 'shaa aHaahl | C.LEVEL | 0 - 100 |
| 4CH 'shaa masahl | L.LEVEL | 0 - 100 |
| 4DH doaa masahl | R-LEVEL | 0 - 100 |
| 4EH doaa masahl | T.NACK | 0 - 100 |
| 4FH-DOOb DObaH | CSTDFF WEG | 0 - 200 (500Hz - | Banks TREE) |
| 50H doaa masahl | CSTDFF LSN | 0 - | Banks TREE) |
| 52H-000D masahl REVERB | DECAY | 0 - | |
| 53H-0000 masahl | NOISE | 0 - 30 Dc, 30, Ec 1 | |
| 54H doaa masahl | | 0 - | (RODM_/POOC_/PODD_ HALL1_/_HALL2_/_HALL3 FLATE1_/ FLATE2_/_FLATE3 SPAIDS2) |
| 52H-0000 DObaH | CSTDFF WEG | 0 - 200 (500Hz - | Banks TREE) |
| 54H-040aa masahl | CSTDFF LSN | 0 - | Banks TREE) |
| 53H-04000 masahl | P.DELAY | 0 - | |
| 56H doaa masahE | L.LEVEL | 0 - 100 |
| 57H-04aaa masahlLINEDDT FILTER | PRESENCE | 0 - 100 |
| 58H 'agaaa masahl | THEBLL | 0 - 100 |
| 59H 'agaaa masahl | LEVEL | 0 - 100 |
| 5AH 'agaaa masahl | BASS | 0 - 100 |
| 5BH doaa masahlMASTER VOLUME | 0 - 100 |
| 5CH 'baaa masahl EXPRESSION ASSIGN GR1 | 0 - 79 (-127 |(Table 4)) |

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

### Continued Mappings

| Address | Description | Reference |
|---------|-------------|-----------|
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

---

## Table 4: Offset Address Configurations

### Memory Area Blocks

| Offset | Description | Parameters |
|--------|-------------|------------|
| SCH-0000 00008 0 | CNFPRISON | TOTL |
| 000D-00019 1 | | ATTACH |
| 000D-00105 2 | | SUSTART |
| 000H-00106 3 | | LEVEL |
| 000H-00106 5 | DISTORTION | D2-00-D0 |
| 000H-00106 6 | | LEVEL |
| D00D-100H 7 | OUTHATCH | D2-00-D0 |
| D00D-100H 8 | | EDUTS |
| D00H-100E 9 | | PHANG FILTER |
| 000H-101H 10 | | LEVEL |
| D00H-101H 11 | | LEVEL |
| D00H-101H 12 | | 0 |
| D00H-101H 14 | | UPHSXS |
| D00H-A5019 15 | STEP PANZER | LEVEL |
| D00H-A000D 16 | | UPTH |
| D00H-A001D 17 | | MANLAL |
| D00H-AA101 18 | | RESONANCE |
| D00H-AA110 19 | | TAP DELAY |
| D00H-D100H 20 | PARAMETRIC E: | HI FREC |
| D00H-01D16 21 | | HI LEVEL |
| D00H-01106 22 | | D-W UREQ |
| 4001-100sB 23 | | D-W LEV |
| 4001-100sB 24 | | D-W LEV |
| 4001-1000E 25 | | L-W FTEC |
| 40D1-101sB 26 | | L-W FREC |
| 4001-101sH 27 | | LO FTEC |
| 4001-1000H 28 | | LO FREC |
| 4001-100sH 29 | | LO LEVEL |
| 4001-1101B 30 | | DST LAY |
| 4001-1100E 31 | SDIST SUPPRESSION | RELEASE |
| 0A01D-D005H 32 | | LEVEL |
| 0A01D-0A01H 33 | | LEVEL |
| 0A01D-0A01H 34 | SHORT DELAY | SHIFT DELAY |
| 0A01D-0A01H 35 | | E.LEVEL |
| 0A01D-D100H 36 | CHDRLS | F DELAY |
| 0A01D-0A101 37 | | RATE |
| 0A01D-0A11H 38 | | DEPTH |
| 0001D-0000H 39 | CHDRLS | DEPTH |
| 0001D-0000H 40 | FLANGER | RATE |
| 0001D-0001E 41 | | DEPTH |
| 0001D-0101H 42 | | RESONANCE |
| 0001D-100sH 43 | PITCH SHIFTER | BALANCE |
| 0001D-100sH 44 | | CHROMATIC |
| 0001D-110sB 45 | | FINE |
| 0001D-110sH 46 | | F.BACK |
| 0001D-000sB 47 | | |
| 0001D-0001E 48 | SPACE D | |
| 0001D-0011H 49 | AUTO PANPOT | |
| 0001D-0011H 50 | | |
| D001D-0E11H 51 | | DEPTH |

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

### MIDI System Mappings

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
