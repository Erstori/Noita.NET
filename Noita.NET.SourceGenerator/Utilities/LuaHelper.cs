namespace Noita.NET.SourceGenerator.Utilities
{
    public static class LuaHelper
    {
        //nil, boolean, number, string, function, userdata, thread, and table
        public static string TypeLua2Net(string typeName)
        {
            switch (typeName)
            {
                case "boolean": return "bool";
                case "string": return "string";
                case "number": return "double";
                case "int|number": return "double";
                case "int": return "int";
                default:
                    break;
            }
            return typeName;
        }
    }
}
