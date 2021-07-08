using System.Collections.Generic;

namespace Noita.NET.SourceGenerator.Utilities
{
    public ref struct DocumentParser
    {
        private List<Parameter> parameters;
        private ReadOnlySpan<char> text;
        private int i;
        public NoitaAPI Current;

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public DocumentParser(ReadOnlySpan<char> document)
        {
            parameters = new List<Parameter>();
            text = document
                .SliceAfter("---".AsSpan())
                .Trim(new[] { ' ', '\r', '\n' });
            i = 0;
            Current = default;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool MoveNext()
        {
            int lineEnd = text.IndexOf('\n');
            if (lineEnd == -1) goto retFalse;

            var line = text.Slice(0, lineEnd).TrimEnd('\r');
            
        retFalse:
            Current = default;
            return false;
        }
    }
}
