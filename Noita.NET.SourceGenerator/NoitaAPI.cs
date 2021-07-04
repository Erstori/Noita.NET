using System.Collections.Generic;

namespace Noita.NET.SourceGenerator
{
    public readonly ref struct NoitaAPI
    {
        public readonly string Name;

        public readonly string Comment;

        public readonly Parameter Result;

        public readonly IList<Parameter> Parameters;

        public NoitaAPI(string name, string comment, Parameter result, List<Parameter> parameters)
        {
            Name = name;
            Comment = comment;
            Result = result;
            Parameters = parameters;
        }
    }

    public record struct Parameter(string Name, string Type)
    {
        public readonly bool IsVoid => Type is null && Name is null;
    }
}
