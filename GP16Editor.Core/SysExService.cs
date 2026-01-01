// GP16Editor/Services/SysExService.cs
using System;
using System.Collections.Generic;
using System.Linq;

namespace GP16Editor.Core
{
    public class SysExService
    {
        // Manufacturer ID for Roland
        private const byte MANUFACTURER_ID = 0x41;
        // Model ID for GP-16
        private const byte MODEL_ID = 0x2A;
        // Command ID for Data Set 1 (DT1)
        private const byte COMMAND_ID_DT1 = 0x12;
        // Command ID for Request Data 1 (RQ1)
        private const byte COMMAND_ID_RQ1 = 0x11;

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

            if (checksum == 128)
            {
                checksum = 0;
            }

            return checksum;
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
    }
}
