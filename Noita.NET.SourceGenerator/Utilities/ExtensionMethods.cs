namespace Noita.NET.SourceGenerator
{
    public static class ExtensionMethods
    {
        public static ReadOnlySpan<char> SliceAfter(this ReadOnlySpan<char> span, ReadOnlySpan<char> value)
        {
            int index = span.IndexOf(value, StringComparison.Ordinal);
            if (index == -1) return default;
            return span.Slice(index + value.Length);
        }
    }
}
