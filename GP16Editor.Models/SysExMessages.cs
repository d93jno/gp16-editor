// GP16Editor.Models/SysExMessages.cs
namespace GP16Editor.Models
{
    public enum BulkDumpType
    {
        Number,
        Bank,
        Group,
        All
    }

    public class DT1Address
    {
        public bool IsVerifiable { get; set; }
        public BulkDumpType DumpType { get; set; }
        public bool IsTemporaryMemory { get; set; }
        public int PatchNumber { get; set; }
        public int ParameterAddress { get; set; }
    }

    public class ParsedDT1Message
    {
        public DT1Address Address { get; set; }
        public byte[] Data { get; set; }
        public bool IsValid { get; set; }

        public ParsedDT1Message()
        {
            Address = new DT1Address();
            Data = new byte[0];
        }
    }
}
