namespace GP16Editor.Models
{
    public class PatchListItem
    {
        public string Id { get; set; } = "";
        public string Name { get; set; } = "";
        public int PatchNumber { get; set; }

        public string DisplayName => $"{Id} - {Name}";
    }
}
