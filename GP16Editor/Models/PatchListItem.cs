namespace GP16Editor.Models
{
    public class PatchListItem
    {
        public string Id { get; set; } = "";
        public string Name { get; set; } = "";
        public int PatchNumber { get; set; }

        public string DisplayName => $"{Id} - {Name}";

        public PatchListItem() { }

        public PatchListItem(Patch patch)
        {
            PatchNumber = patch.PatchNumber;
            Name = patch.PatchName;
            var group = (PatchNumber - 1) < 64 ? "A" : "B";
            var patchInGroup = (PatchNumber - 1) % 64;
            var bank = patchInGroup / 8 + 1;
            var patchInBank = patchInGroup % 8 + 1;
            Id = $"{group}{bank}{patchInBank}";
        }
    }
}
