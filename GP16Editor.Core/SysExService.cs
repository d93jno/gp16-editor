// GP16Editor/Services/SysExService.cs
using System;
using System.Collections.Generic;
using System.Linq;

namespace GP16Editor.Core
{
    public class SysExService
    {
        // Manufacturer ID for Roland
        public const byte MANUFACTURER_ID = 0x41;
        // Model ID for GP-16
        public const byte MODEL_ID = 0x2A;
        // Command ID for Data Set 1 (DT1)
        public const byte COMMAND_ID_DT1 = 0x12;
        // Command ID for Request Data 1 (RQ1)
        public const byte COMMAND_ID_RQ1 = 0x11;

        /// <summary>
        /// Calculates the Roland 7-bit checksum for SysEx messages.
        /// </summary>
        /// <param name="data">The address and data bytes for which to calculate the checksum.</param>
        /// <returns>The calculated 7-bit checksum byte.</returns>
        public byte CalculateChecksum(IEnumerable<byte> data)
        {
            if (data == null)
            {
                throw new ArgumentNullException(nameof(data));
            }

            long sum = data.Sum(b => (long)b);
            byte remainder = (byte)(sum % 128);
            byte checksum = (byte)(128 - remainder);

            return (byte)(checksum % 128);
        }

        // Method to build a DT1 (Data Set 1) message
        public IEnumerable<byte> BuildDt1Message(byte deviceId, byte[] addressBytes, byte[] dataBytes)
        {
            if (addressBytes == null || addressBytes.Length != 3)
            {
                throw new ArgumentException("Address bytes must be a 3-byte array.", nameof(addressBytes));
            }
            if (dataBytes == null || dataBytes.Length == 0)
            {
                throw new ArgumentException("Data bytes cannot be null or empty.", nameof(dataBytes));
            }

            var messageBody = new List<byte>();
            messageBody.AddRange(addressBytes);
            messageBody.AddRange(dataBytes);

            byte checksum = CalculateChecksum(messageBody);

            var sysexMessage = new List<byte>
            {
                0xF0, // SysEx Start
                MANUFACTURER_ID,
                deviceId,
                MODEL_ID,
                COMMAND_ID_DT1
            };
            sysexMessage.AddRange(messageBody);
            sysexMessage.Add(checksum);
            sysexMessage.Add(0xF7); // SysEx End

            return sysexMessage;
        }

        // Method to build an RQ1 (Request Data 1) message
        public IEnumerable<byte> BuildRq1Message(byte deviceId, byte[] addressBytes, byte[] sizeBytes)
        {
            if (addressBytes == null || addressBytes.Length != 3)
            {
                throw new ArgumentException("Address bytes must be a 3-byte array.", nameof(addressBytes));
            }
            if (sizeBytes == null || sizeBytes.Length != 3)
            {
                throw new ArgumentException("Size bytes must be a 3-byte array.", nameof(sizeBytes));
            }

            var messageBody = new List<byte>();
            messageBody.AddRange(addressBytes);
            messageBody.AddRange(sizeBytes);

            byte checksum = CalculateChecksum(messageBody);

            var sysexMessage = new List<byte>
            {
                0xF0, // SysEx Start
                MANUFACTURER_ID,
                deviceId,
                MODEL_ID,
                COMMAND_ID_RQ1
            };
            sysexMessage.AddRange(messageBody);
            sysexMessage.Add(checksum);
            sysexMessage.Add(0xF7); // SysEx End

            return sysexMessage;
        }

        public Models.ParsedDT1Message ParseDt1Message(byte[] sysexData)
        {
            var parsedMessage = new Models.ParsedDT1Message();

            // Basic validation for a DT1 message
            // F0 41 dev 2A 12 adr adr adr data... chk F7
            if (sysexData == null || sysexData.Length < 9 || // Minimal DT1 is F0 41 dev 2A 12 adr adr adr chk F7 (no data)
                sysexData[0] != 0xF0 ||
                sysexData[sysexData.Length - 1] != 0xF7 ||
                sysexData[1] != MANUFACTURER_ID ||
                sysexData[3] != MODEL_ID ||
                sysexData[4] != COMMAND_ID_DT1)
            {
                parsedMessage.IsValid = false;
                return parsedMessage;
            }

            // Extract address and data
            var addressBytes = sysexData.Skip(5).Take(3).ToArray();
            var dataBytes = sysexData.Skip(8).Take(sysexData.Length - 10).ToArray();

            // Verify checksum
            var messageBody = new List<byte>();
            messageBody.AddRange(addressBytes);
            messageBody.AddRange(dataBytes);

            var calculatedChecksum = CalculateChecksum(messageBody);
            var receivedChecksum = sysexData[sysexData.Length - 2];

            if (calculatedChecksum != receivedChecksum)
            {
                parsedMessage.IsValid = false;
                return parsedMessage;
            }

            parsedMessage.IsValid = true;
            parsedMessage.Data = dataBytes;

            // Parse address bytes from the 3-byte address field
            var msb = addressBytes[0]; 
            
            parsedMessage.Address.IsVerifiable = (msb & 0b01000000) != 0;
            parsedMessage.Address.DumpType = (Models.BulkDumpType)((msb >> 4) & 0b00000011);
            parsedMessage.Address.IsTemporaryMemory = (msb & 0b00001000) != 0;

            parsedMessage.Address.PatchNumber = addressBytes[1] & 0x7F; // 7-bit value
            parsedMessage.Address.ParameterAddress = addressBytes[2] & 0x7F; // 7-bit value

            return parsedMessage;
        }
    }
}
