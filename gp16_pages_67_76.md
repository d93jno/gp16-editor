# Roland Exclusive Messages

## 1. Data Format for Exclusive Messages

Roland's MIDI implementation uses IPC (InterProcessor Communication) protocol for all exclusive messages (Type IV).

| Byte | Description |
|------|-------------|
| F0H | Exclusive status |
| 41H | Manufacturer ID (Roland) |
| DEV | Device ID |
| MDL | Model ID |
| CMD | Command ID |
| [BODY] | Main Data |
| F7H | End of exclusive |

### = MIDI status - F0H, F7H

An exclusive message must be flanked by a pair of status bytes - one (F0H) starting with a Manufacturer ID immediately after the MIDI version1.0).

### = Manufacturer ID - 41H

The Manufacturer ID identifies the manufacturer of a MIDI instrument that requires an Exclusive message. Value 41H represents Roland's Manufacturer ID.

### = Device ID - DEV

The Device ID contains a unique value that identifies the individual MIDI device that will multiple instrumentation in a B multi-instrument environment. One should not confuse this with multiple basic channels. Channel and IFLI are to be used with multiple basic channels.

### = Model ID - MDL

The Model ID contains a value that uniquely identifies one model from another. Different models, however, may share an identical Model ID since they handle identical data.

The Model ID format may contain 00H in one or more places to provide an extended data field. The following are examples of formats in such cases representing a unique Model ID:

- 01H
- 00H, 01H
- 00H
- 00H, 01H
- 00H, 02H
- 00H, 00H, 01H

### = Command ID - CMD

The Command ID indicates the function of an exclusive message. The Command ID format may contain 00H in one or more places to provide an extended data field. The following are examples of valid Command ID values, all of which are representing a unique function:

- 01H
- 02H
- 00H
- 01H, 01H
- 00H, 02H
- 00H, 00H, 01H

### = Main data - BODY

This field contains a message to be exchanged across an interface. The exact data size and contents will vary with the Model ID and Command ID.

---

## 2. Address mapped Data Transfer

Address mapping is a technique for transferring messages conforming to the data format given in Section 1. It assigns a series of memory-resident records, system programs, and user data in a device to an address within address space, thereby allowing access to data residing at the address by specifying it in a machine-independent address space, thereby allowing access to data residing at the address of a message specifies.

Address mapped data transfer is interdependent of models and data categories. This technique allows one or two different transfer procedures: one-way transfer and handshake transfer.

---

## 3. One-way Transfer Procedure

**See Section 3 for details:**

This procedure is suited for the transfer of a small amount of data. It sends out an entire message (command) independent of a receiving device status.

### Connection Diagram

```
Device (A)                    Device (B)
MID OUT ----------------> MIDI IN
MIDI IN <---------------- MIDI OUT
```

Connection at point 2 is essential for **Request data** procedures. (See Section 4)

---

## 4. Handshake Transfer Procedure

**See Section 4 for details:**

This procedure initiates a predetermined transfer sequence accompanying automatic exchange, eliminating polling of data. Handshaking ensures that registers data transfers, and transfer speed are high enough to handle fast transfer of data.

### Connection Diagram

```
Device (A)                    Device (B)
MIDI OUT ----------------> MIDI IN
MIDI IN  <---------------- MIDI OUT
```

Connection at points 1 and 2 is essential.

### Notes on the above two procedures

- There are separate Command IDs for different transfer procedures.
- For Device (B) of similar specifications and using the same transfer procedure: share identical Device ID and Model ID; and are ready for communicate.

---

## 3. One-way Transfer Procedure

This procedure sends out data all the way until it stops and is used when the messages are to share that awesomeness need not be checked.

Each message is transmitted with a known length or total amount. Each message in time with the transfer sequence. Waits on intervals of at least 20 milliseconds.

### Types of Messages

| Message | Command ID |
|---------|------------|
| Request data 1 | RQ1 (11H) |
| Data set 1 | DT1 (12H) |

---

### = Request data 1 - RQ1 (11H)

This message is sent out when there is a need to acquire data from a device at the other end of the interface. It contains data for the address and size that specifies that requested data (address format and size of each model ID must match).

On receiving an RQ1 message, the remote device checks its memory for the data address, and size that which were sent. It then transmits a "Data set 1 (DT1)" message.

If it finds them and is ready for communication, the device will transmit a "Data set 1 (DT1)" message, which contains the requested data. Otherwise, the device will send out nothing.

| Byte | Description |
|------|-------------|
| F0H | Exclusive status |
| 41H | Manufacturer ID (Roland) |
| DEV | Device ID |
| MDL | Model ID |
| 11H | Command ID |
| aaH | Address MSB |
| : | : |
| : | : |
| LSB | |
| ssH | Size MSB |
| : | : |
| : | : |
| LSB | |
| sum | Check sum |
| F7H | End of exclusive |

**Notes:**
- The size of the requested data does not indicate the number of bytes that will make up a "Data set (DAT)" message, but represents the address fields where the requested data resides.
- Some models are subject to limitations in data format used for a single transmission. Requested data, for example, may have a limit in length or must be divided into predetermined address fields before it is exchanged across the interface.
- The same number of bytes comprised address data varies from one model ID to another.
- The error checking process uses a checksum that provides full return of the data significant 7 bits are zero when values for an address, size, and that checksum are summed.

---

### = Data set 1 - DT1 (12H)

This message corresponds to the actual data transfer process. Because every byte in the data is assigned a unique address, a DT1 message can convey the starting address of one or more data sets along with its (their) contents (all in a dependent format.

The MIDI standard inhibits any real time messages from interrupting an exclusive one. This fact is inconvenient for the implementation that support a "bulk-dumped" mechanism. To maintain compatibility with such devices, Roland has limited the DT1 to 256 bytes so that an exclusively long message is sent out in separate segments.

| Byte | Description |
|------|-------------|
| F0H | Exclusive |
| 41H | Manufacturer ID (Roland) |
| DEV | Device ID |
| MDL | Model ID |
| 12H | Command ID |
| aaH | Address MSE |
| : | : |
| : | : |
| LSB | |
| ddH | Data |
| : | : |
| : | : |
| sum | Check sum |
| F7H | End of exclusive |

**Notes:**
- A DT1 message is capable of providing only the valid data among those specified by an RQ1 message.
- Some models are subject to limitations in data format used for a single transmission. Requested data, for example, may have a limit in length or must be divided into predetermined address fields before it is exchanged across the interface.
- The number of bytes comprised address data varies from one model ID to another.
- The error checking process uses a checksum that provides full return of the data significant 7 bits are zero when values for an address, size, and that checksum are summed.

### = Example of Message Transactions

**● Device A sending data to Device B**

First establish a DT1 message(s) to all that takes place.

```
[Device (A)]                    [Device (B)]
[Data set 1] ----------------->
        ★ Move than 20ms see time interval
[Data set 1] ----------------->
        :
        :
[Data set 1] ----------------->
```

---

## 4. Handshake Transfer Procedure

This procedure employs a predetermined transfer sequence to exchange error checking signals before a message transaction takes place, thereby increasing data reliability. Unlike one-way transfer that inserts a pause between message transactions, handshake transfer almost never pauses (except immediately following a handshake), thus much speedier transactions because of no pause necessary (provided all conditions below are met) to exchange a complete set of messages, for instance across a MIDI interface, handshaking transfer is more efficient than one-way transfer.

When it comes to handshake type of data transfer, one data segment is sent at a time, with a checksum is exchanged, and synthesizer tones sent over the entire range, for instance across a MIDI interface, handshaking becomes a much more efficient protocol.

### Types of Messages

| Message | Command ID |
|---------|------------|
| Want to send data | WSD (40H) |
| Request data | RQD (41H) |
| Data set | DAT (42H) |
| Acknowledge | ACK (43H) |
| End of data | EOD (45H) |
| Communication error | ERR (4EH) |
| Rejection | RJC (4FH) |

---

### = Want to send data - WSD (40H)

This message is sent out when data must be sent to a device at the other end of the interface. It contains data for the address and size that specify destination and length.

On receiving a WSD message, the remote device checks its memory for the specified data address and size which were previously mentioned when issuing error checking, which will eventually be followed by communication, the device will return an "Acknowledge (ACK)" message.

| Byte | Description |
|------|-------------|
| F0H | Exclusive status |
| 41H | Manufacturer ID (Roland) |
| DEV | Device ID |
| MDL | Model ID |
| 40H | Command ID |
| aaH | Address MSB |
| : | : |
| : | : |
| LSB | |
| ssH | Size MSB |
| : | : |
| : | : |
| LSB | |
| sum | Check sum |
| F7H | End of exclusive |

**Notes:**
- Otherwise, it will return a "Rejection (RJC)" message.
- The size of the data to be sent does not indicate the number of bytes that make up a "Data set (DAT)" message, but represents the address fields where the requested data is to reside.
- Some models are subject to limitations in data format used for a single transmission. Requested data, for example, may have a limit in length or must be divided into predetermined address fields before it is exchanged across the interface.
- The same number of bytes comprised address data varies from one model ID to another, however, vary with the Model ID.
- The error checking process uses a checksum that provides full return of the data significant 7 bits are zero when values for an address, size, and that checksum are summed.

---

### = Request data - RQD (41H)

This message is sent out when there is a need to acquire data from a device at the other end of the interface. It contains data for the address and size that specifies that requested data (address format and size of each model ID vary).

On receiving an RQD message, the remote device checks its memory for the data address, and size which came in. If it finds them and is ready for communication, the device will transmit a "Data set (DAT)" message. What remains the requested data. Otherwise, it will return a "Rejection (RJC)" message.

| Byte | Description |
|------|-------------|
| F0H | Exclusive status |
| 41H | Manufacturer ID (Roland) |
| DEV | Device ID |
| MDL | Model ID |
| 41H | Command ID |
| aaH | Address MSB |
| : | : |
| : | : |
| LSB | |
| ssH | Size MSB |
| : | : |
| : | : |
| LSB | |
| sum | Check sum |
| F7H | End of exclusive |

**Notes:**
- The size of the requested data does not indicate the number of bytes that make up a "Data set (DAT)" message, but represents the address fields where the requested data will reside.
- Some models are subject to limitations in data format used for a single transmission. Requested data, for example, may have a limit in length or must be divided into predetermined address fields before it is exchanged across the interface.
- The same number of bytes comprised address data varies from one model ID to another.
- The error checking process uses a checksum that provides full return of the data significant 7 bits are zero when values for an address, size, and that checksum are summed.

---

### = Data set - DAT (42H)

This message corresponds to the actual data transfer process. Because every byte in the data is assigned a unique address, a DAT message can convey the starting address of one or more data sets along with its (their) contents.

Although the MIDI standards inhibit non real time messages from interrupting an exclusive one, such devices support a "bulk-dumped" mechanism has limited length of 256 bytes so that an exclusively long message is not necessary to separate segments into units. For compatibility with such devices, Roland has limited the DAT to 256 bytes so that an exclusively long message is sent out in separate segments.

| Byte | Description |
|------|-------------|
| F0H | Exclusive status |
| 41H | Manufacturer ID (Roland) |
| DEV | Device ID |
| MDL | Model ID |
| 42H | Command ID |
| aaH | Address MSB |
| : | : |
| : | : |
| LSB | |
| ddH | Data |
| : | : |
| : | : |
| sum | Check sum |
| F7H | End of exclusive |

**Notes:**
- A DAT message is capable of providing only the valid data.
- Some models are subject to limitations in data format used for a single transmission. Requested data, for example, may have a limit in length or must be divided into predetermined address fields before it is exchanged across the interface.
- The same number of bytes comprised address data varies from one model ID to another.
- The error checking process uses a checksum that provides full return of the data significant 7 bits are zero when values for an address, size, and that checksum are summed.

---

### = Acknowledge - ACK (43H)

This message is sent out when no error was detected on reception of a WSD, DAT, or all data (EOD)", or when there is no need for any particular action, it receives an ACK message, the device at the other end will not proceed to the next operation.

| Byte | Description |
|------|-------------|
| F0H | Exclusive status |
| 41H | Manufacturer ID (Roland) |
| DEV | Device ID |
| MDL | Model ID |
| 43H | Command ID |
| F7H | End of exclusive |

---

### = End of data - EOD (45H)

This message is sent out to inform a remote device of the end of a message. Communication, however, will not come to an end unless the remote device returns an ACK message even though an EOD message was transmitted.

| Byte | Description |
|------|-------------|
| F0H | Exclusive status |
| 41H | Manufacturer ID (Roland) |
| DEV | Device ID |
| MDL | Model ID |
| 45H | Command ID |
| F7H | End of exclusive |

---

### = Communications error - ERR (4EH)

This message warns the remote device of a communications fault encountered during message transmission (no, for example, a checksum error, a "Rejection (RJC)" one, which terminates the current message). An ERR message.

When it receives an ERR message, the remote device may either attempt to send out the last message it already has or send out an "Acknowledge (ACK)" message.

| Byte | Description |
|------|-------------|
| F0H | Exclusive status |
| 41H | Manufacturer ID (Roland) |
| DEV | Device ID |
| MDL | Model ID |
| 4EH | Command ID |
| F7H | End of exclusive |

---

### = Rejection - RJC (4FH)

This message is sent out when there is a need to terminate communication to overcome the current message: An RJC message when the following conditions are met:

- a WSD or RQD message has specified an illegal data address or size.
- the device is not ready for communication.
- an illegal number of addresses or calls has been detected.
- Data transfer has been terminated by an operator.
- A communication error has occurred.

An ERR message may be sent out by a device on either end of the interface. Communication must be terminated at this message was received. Communication can be restarted after this message.

| Byte | Description |
|------|-------------|
| F0H | Exclusive status |
| 41H | Manufacturer ID (Roland) |
| DEV | Device ID |
| MDL | Model ID |
| 4FH | Command ID |
| F7H | End of exclusive |

---

## = Example of Message Transactions

### ● Data transfer from device (A) to device (B).

```
[Device (A)]                    [Device (B)]

[Want to send data] ---------->
                    <---------- [Acknowledge]
[Data set] -------------------->
                    <---------- [Acknowledge]
[Data set] -------------------->
                    <---------- [Acknowledge]
        :
        :
[End of data] ---------------->
                    <---------- [Acknowledge]
```

---

### ● Device (A) requests and receives data from device (B).

```
[Device (A)]                    [Device (B)]

[Request data] --------------->
                    <---------- [Data set]
[Acknowledge] ---------------->
                    <---------- [Data set]
[Acknowledge] ---------------->
        :
        :
                    <---------- [End of data]
[Acknowledge] ---------------->
```

---

### ● Error occurs while device (A) is receiving data from device (B).

#### 1) Data transfer from device (A) to device (B).

```
[Device (A)]                    [Device (B)]

        :
        :
                    <---------- [Data set]
[Error] ---------------------->
                    <---------- [Data set]
[Communication error] --------->
                    <---------- [Data set]
                                 (the same data
                                  as above)
[Acknowledge] ---------------->
```

#### 2) Device (B) rejects the data re-transmitted, and quits data transfer

```
[Device (A)]                    [Device (B)]

        :
        :
                    <---------- [Data set]
[Error] ---------------------->
                    <---------- [Data set]
[Communication error] --------->
                    <---------- [Rejection]
```

#### 3) Device (A) immediately quits data transfer.

```
[Device (A)]                    [Device (B)]

        :
        :
[Acknowledge] ---------------->
                    <---------- [Data set]
[Rejection] ------------------>
                    <---------- [Data set]
                                 (Quit)
```

---

**Page 71**

# DIGITAL GUITAR EFFECTS PROCESSOR
# Model GP-16
# MIDI Implementation
**Date: Aug. 21 1989**
**Version: 1.01**

---

## 1. TRANSMITTED DATA

### Control Change

| Status | Second | Third |
|--------|--------|-------|
| BnH | 00H | val |
| n = MIDI Basic Channel | 01 | PH (1 - 16) |
| c = Control Number | 000 | PH (0 - 127) |
| c = Control Value | 00H | 7PH (0 - 127) |

When the TRANSMIT MIDI CONTROL MODE of the Unit is CONTROL, the GP-16 cc off. the GP-16 converts messages received from external MIDI devices designated for General Purpose Controller 1 with Controller Number 16 for button "1": SYSTEM INFORMATION" TRANSMIT" CONTROL" VOLUME" for button.

### Program Change

| Status | Second |
|--------|--------|
| CnH | nnH |
| n = MIDI Basic Channel | 01 - PH (1 - 127) |
| = Program Number | 00H - 7FH (0 - 127) |

When the current patch is changed by the front panel switches, the GP-16 transmits MIDI messages with the Program Number corresponding to the new patch.

### System Exclusive

| Status | |
|--------|---|
| F0H | |
| F7H | EOX (End Of System Exclusive) |

The GP-16 transmits two System Exclusive formats: the BULK DUMP function. Upon receiving a command for System Exclusive, the GP-16 is executed the Bulk Dump function. For more details please refer to "3. EXCLUSIVE COMMUNICATIONS" and "Roland Exclusive Messages" in this manual.

---

## 2. RECOGNIZED RECEIVE DATA

### Program Change

| Status | Second |
|--------|--------|
| CnH | nnH |
| n = MIDI Basic Channel | 01 - PH (1 - 16) |
| = Program Number | 00H - 7FH (0 - 127) |

The patch can be recalled according to the Program Number of the message received.

### Control Change

#### Main Volume

| Status | Second | Third |
|--------|--------|-------|
| BnH | 07H | val |
| n = MIDI Basic Channel | 01 - PH (1 - 16) |
| = Control Number | 07H | 00H - 7FH (0 - 127) |

Setting the EXPRESSION DEVICE parameter to "PEDAL" allows a specified expression device to be activated.

#### General Purpose Controller 1

| Status | Second | Third |
|--------|--------|-------|
| BnH | 50H | val |
| n = MIDI Basic Channel | 01 - PH (1 - 16) |
| = Control Value | 00H - 3FH (0 - 63) : OFF |
|  |  | 40H - 7FH (64 - 127) : ON |

When the System Mode No.5 "MIDI Func" is "RECEIVE", the Output Mode function can be turned on or off. When this mode is set to "BYPASS", the output can be switched between being bypassed or effected.

#### General Purpose Controller 5

| Status | Second | Third |
|--------|--------|-------|
| BnH | 50H | val |
| n = MIDI Basic Channel | 01 - PH (1 - 16) |
| = Control Value | 00H - 3FH (0 - 63) |

### System Exclusive

| Status | |
|--------|---|
| F0H | |
| 7FH | EOX (End Of System Exclusive) |

The GP-16's parameter settings or temporary data can be requested or edited to other MIDI devices. For more details, please refer to "3. EXCLUSIVE COMMUNICATIONS" and "Roland Exclusive Messages" in this manual.

---

## 3. EXCLUSIVE COMMUNICATIONS

Via Exclusive Messages, the GP-16 can send or receive parameter settings data in either of two ways:

In the memory section of the GP-16, a temporary area is provided as a buffer for parameter downloading. When necessary, this buffer should be filled with an internal memory bank and its corresponding parameters areas stored in the internal memory starting 128 patches and parameter settings. The buffer contains data memory in.

The temporary buffer function allows Buffer data of the GP-16 to be both dumped. Also, internal memory data can be stored at the buffer.

### Bulk Dump Function

Like the Data Sending operation to prepare the GP-16 for accepting data from an external MIDI device. After receiving a System Exclusive Message, the GP-16 if arrives in the proper format through recognition using the model at the buffer so as to receive data. On the other hand, if the data sent to the GP-16 is in the Play Mode, in addition, the data verification operation shows that the GP-16 sends the memory sequence value of the transferred data is not available.

Exclusive communications of the GP-16 is always conducted under the following two-way communication format (known as the Roland Exclusive Format, type IV) For more details, please refer to "Roland Exclusive Messages" in this manual.

### Request: (One way) RQ1 11H

After receiving the Request Data message, the GP-16 replies the Data Set (DT1) to the port that sent the command. If the content is specified by the address and size in the received Data Address.

For the Data ID, numbers one unit lower than each MIDI Channel number are used. The GP-16 does not transmit this message.

| Byte | Description |
|------|-------------|
| F0H | Exclusive status |
| 41H | Manufacturer ID (Roland) |
| Device ID : i - 0H - FH (1CH - 16CH) |
| 2AH | Model ID (GP-16) |
| 11H | Command ID (RQ1) |
| aaH | Address |
| aaH | Address MSB |
| aaH | Address |
| ssH | Address LSB |
| ssH | Size |
| ssH | Size MSB |
| ssH | Size LSB |
| sum | Checksum : < (ignore if received by the GP-16) > |
| F7H | End of System Exclusive |

---

### Data set: (One way) DT1 12H

Depending on the type of data to be received, the GP-16 accepts this message in two cases:

#### [For normal memory data]

When User Switch No. 6 "MIDI Request" is ON, the GP-16 is ready to accept the default parameter sent from an external MIDI device. Other appropriate data within the internal memory unit to the Play Mode.

#### [For temporary buffer data]

The GP-16 is usually accepts parameter MIDI data both in the Play and condition and in the Play Mode.

This message can be transmitted in the following cases:

- Data communications between the GP-16 addresses the specific data parameter among those stored from an external MIDI device.
- When the GP-16 is made to execute the Bulk Dump function, it transmits the parameter settings of one specific patch.

For the device ID, numerals one unit lower than each MIDI Channel number are used.

| Byte | Description |
|------|-------------|
| F0H | Exclusive status |
| 41H | Manufacturer ID (Roland) |
| DEV | Device ID : i - 0H - FH (1CH - 16CH) |
| 2AH | Model ID (GP-16) |
| 12H | Command ID (DT1) |
| aaH | Address MSB |
| aaH | Address |
| aaH | Address LSB |
| ddH | Data |
| sum | Checksum |
| F7H | End of System Exclusive |

---

## 4. ADDRESS MAPPING OF PARAMETERS

This address mapping is 1, 2, 3 hexadecimal address.

| Address | Area | Description |
|---------|------|-------------|

### Patch Data Area

| Address | Description | Offset |
|---------|-------------|--------|
| Patch Num. | 00 | Patch Number |
| Effect ID | 01-05 | Effect Number |
| E.EQ | | |
|  |  | 06-0A | Effect Number Group # |
|  | Memory Bank GroupB | |
|  | Number Bank Group # 0-7 |  |

#### E. Description

**EFFECT MIX**

All data sent to the GP-16 is Read on, written into memory, or for internal unit which may have changed or be discussed.

There are two ways that data transferred is enclosed in the external memory. First, all data are transferred by memory space. The GP-16 must be in the various settings available. All data sent to the GP-16 is Read on written into memory settings or the parameters to the value of this range when the GP-16 is in correct situation to receive the value etc.

#### On: Bank Area Type

Memory: Normal Mode

If: Memory Bank Group (0-7)

Temporary: Normal Bank

The actual structure of effect parameter at the Start Address figure for each block. (See the Extract Address figure.)

#### Temporary Area

**BLOCK** is a concept.

**EXTRACT** is a concept.

All possible data in each change or type chain key parameter settings. A key patch number will be sensed and grouped data from the same parameter structure to same Group of the GP-16, since to the variable parameter values. Causes data such as parameter and multiple decisions teach section containing data with the same parameter values. Output data be unchanged.

#### Data

| Address | Description | Values | |
|---------|-------------|--------|---|

[Large detailed address mapping table with hexadecimal values, parameters, and offsets follows - contains extensive technical data about memory addresses, effect parameters, and system settings]

---

## MIDI Implementation Chart

Model GP-16

| Function | Transmitted | Recognized | Remarks |
|----------|-------------|------------|---------|
| **Basic Channel** | Default 1 - 16 | 1 - 16 | Memorized |
|  | Changed 1 - 16 | 1 - 16 | |
| **Mode** | Default × | OMNI ON/OFF | Memorized |
|  | Messages × | × | |
|  | Altered ★★★★★★★★ | × | |
| **Note Number** | True Voice × | × | |
|  |  | ★★★★★★★★★ | × | |
| **Velocity** | Note ON × | × | |
|  | Note OFF × | × | |
| **After Touch** | Key's × | × | |
|  | Ch's × | × | |
| **Pitch Bend** |  | × | × | |
| | | × | × | **Volume** |
|  | 1-6 | ★ 1 | ★ 2 |
|  | 0 - 31 | ★ 1 | × | ★ 3 |
|  | 64 - 95 | ★ 1 | × | ★ 3 |
|  | 96 | × | O | **Mute or Bypass** |
| **Control Change** |  |  | |
| **Prog Change** | True # | O 0 - 127 | O 0 - 127 | |
|  |  | ★★★★★★★★ | 0 - 127 | |
| **System Exclusive** |  | O | | parameter values |
| **System Common** | Song Pos | × | × | |
|  | Song Sel | × | × | |
|  | Tune | × | × | |
| **System Real Time** | Clock | × | × | |
|  | Commands | × | × | |
| **Aux Messages** | Local ON/OFF | × | × | |
|  | All Notes OFF | × | × | |
|  | Active Sense | × | × | |
|  | Reset | × | × | |

### Notes

★ 1: Either "C" or "×" can be selected manually and stored in memory.
★ 2: It is possible to designate a single parameter among various parameter settings for the Effect On message for adjustment of value.
★ 3: The GP-16 receives Control Change Message No.16 which is converted into Controller Number 0( - 31, 64 - 95) for output.

**Mode 1: OMNI ON, POLY**
**Mode 2: OMNI ON, MONO**
**Mode 3: OMNI OFF, POLY**
**Mode 4: OMNI OFF, MONO**

**O: Yes**
**×: No**
