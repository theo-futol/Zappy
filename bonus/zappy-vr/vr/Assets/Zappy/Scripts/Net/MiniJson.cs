using System.Collections.Generic;
using System.Globalization;
using System.Text;

namespace Zappy.Net
{
    /// <summary>
    /// Minimal JSON reader for the bridge's messages: objects become
    /// Dictionary&lt;string, object&gt;, arrays List&lt;object&gt;, numbers double.
    /// Parse-only on purpose — the client never sends JSON.
    /// </summary>
    public static class MiniJson
    {
        public static object Parse(string json)
        {
            if (string.IsNullOrEmpty(json))
                return null;
            int i = 0;
            try
            {
                return ParseValue(json, ref i);
            }
            catch
            {
                return null; // A malformed frame is dropped, not fatal.
            }
        }

        private static object ParseValue(string s, ref int i)
        {
            SkipWhitespace(s, ref i);
            switch (s[i])
            {
                case '{': return ParseObject(s, ref i);
                case '[': return ParseArray(s, ref i);
                case '"': return ParseString(s, ref i);
                case 't': i += 4; return true;
                case 'f': i += 5; return false;
                case 'n': i += 4; return null;
                default: return ParseNumber(s, ref i);
            }
        }

        private static Dictionary<string, object> ParseObject(string s, ref int i)
        {
            var result = new Dictionary<string, object>();
            i++; // '{'
            while (true)
            {
                SkipWhitespace(s, ref i);
                if (s[i] == '}')
                {
                    i++;
                    return result;
                }
                string key = ParseString(s, ref i);
                SkipWhitespace(s, ref i);
                i++; // ':'
                result[key] = ParseValue(s, ref i);
                SkipWhitespace(s, ref i);
                if (s[i] == ',')
                    i++;
            }
        }

        private static List<object> ParseArray(string s, ref int i)
        {
            var result = new List<object>();
            i++; // '['
            while (true)
            {
                SkipWhitespace(s, ref i);
                if (s[i] == ']')
                {
                    i++;
                    return result;
                }
                result.Add(ParseValue(s, ref i));
                SkipWhitespace(s, ref i);
                if (s[i] == ',')
                    i++;
            }
        }

        private static string ParseString(string s, ref int i)
        {
            var builder = new StringBuilder();
            i++; // '"'
            while (s[i] != '"')
            {
                if (s[i] == '\\')
                {
                    i++;
                    switch (s[i])
                    {
                        case 'n': builder.Append('\n'); break;
                        case 't': builder.Append('\t'); break;
                        case 'r': builder.Append('\r'); break;
                        case 'b': builder.Append('\b'); break;
                        case 'f': builder.Append('\f'); break;
                        case 'u':
                            builder.Append((char)int.Parse(s.Substring(i + 1, 4), NumberStyles.HexNumber));
                            i += 4;
                            break;
                        default: builder.Append(s[i]); break;
                    }
                }
                else
                {
                    builder.Append(s[i]);
                }
                i++;
            }
            i++; // closing '"'
            return builder.ToString();
        }

        private static object ParseNumber(string s, ref int i)
        {
            int start = i;
            while (i < s.Length && "0123456789+-.eE".IndexOf(s[i]) >= 0)
                i++;
            return double.Parse(s.Substring(start, i - start), CultureInfo.InvariantCulture);
        }

        private static void SkipWhitespace(string s, ref int i)
        {
            while (i < s.Length && char.IsWhiteSpace(s[i]))
                i++;
        }
    }

    /// <summary>Typed accessors over MiniJson's object graph.</summary>
    public static class Json
    {
        public static Dictionary<string, object> Dict(object value) => value as Dictionary<string, object>;

        public static List<object> List(object value) => value as List<object>;

        public static string Str(object value) => value?.ToString() ?? "";

        public static int Int(object value)
        {
            if (value is double d)
                return (int)d;
            return int.TryParse(Str(value), out int parsed) ? parsed : 0;
        }

        /// <summary>dict[key] or null — never throws.</summary>
        public static object At(Dictionary<string, object> dict, string key)
        {
            return dict != null && dict.TryGetValue(key, out object value) ? value : null;
        }
    }
}
