#pragma once





/** Add the formated string to the existing data in the string.
Returns a_Dst. */
extern KEEN_API_EXPORT AString & AppendVPrintf(AString & a_Dst, const char * format, va_list args);

/** Output the formatted text into the string.
Returns a_Dst. */
extern KEEN_API_EXPORT AString & Printf       (AString & a_Dst, const char * format, ...);

/** Output the formatted text into string
Returns the formatted string by value. */
extern KEEN_API_EXPORT AString Printf(const char * format, ...);

/** Add the formatted string to the existing data in the string.
Returns a_Dst */
extern KEEN_API_EXPORT AString & AppendPrintf (AString & a_Dst, const char * format, ...);

/** Split the string at any of the listed delimiters.
Return the splitted strings as a stringvector. */
extern KEEN_API_EXPORT AStringVector StringSplit(const AString & str, const AString & delim);

/** Split the string at any of the listed delimiters. Keeps quoted content together
Resolves issue #490
Return the splitted strings as a stringvector. */
extern KEEN_API_EXPORT AStringVector StringSplitWithQuotes(const AString & str, const AString & delim);

/** Join a list of strings with the given delimiter between entries. */
extern KEEN_API_EXPORT AString StringJoin(const AStringVector & a_Strings, const AString & a_Delimiter);

/** Split the string at any of the listed delimiters and trim each value.
Returns the splitted strings as a stringvector. */
extern KEEN_API_EXPORT AStringVector StringSplitAndTrim(const AString & str, const AString & delim);

/** Trims whitespace at both ends of the string.
Returns a trimmed copy of the original string. */
extern KEEN_API_EXPORT AString TrimString(const AString & str);

/** In-place string conversion to uppercase.
Returns the same string object. */
extern KEEN_API_EXPORT AString & InPlaceUppercase(AString & s);

/** In-place string conversion to lowercase.
Returns the same string object. */
extern KEEN_API_EXPORT AString & InPlaceLowercase(AString & s);

/** Returns an upper-cased copy of the string */
extern KEEN_API_EXPORT AString StrToUpper(const AString & s);

/** Returns a lower-cased copy of the string */
extern KEEN_API_EXPORT AString StrToLower(const AString & s);

/** Case-insensitive string comparison.
Returns 0 if the strings are the same, <0 if s1 < s2 and >0 if s1 > s2. */
extern KEEN_API_EXPORT int NoCaseCompare(const AString & s1, const AString & s2);

/** Case-insensitive string comparison that returns a rating of equal-ness between [0 - s1.length()]. */
extern KEEN_API_EXPORT size_t RateCompareString(const AString & s1, const AString & s2);

/** Replaces each occurence of iNeedle in iHayStack with iReplaceWith */
extern KEEN_API_EXPORT void ReplaceString(AString & iHayStack, const AString & iNeedle, const AString & iReplaceWith);

/** Converts a stream of BE shorts into UTF-8 string; returns a_UTF8. */
extern KEEN_API_EXPORT AString & RawBEToUTF8(const char * a_RawData, size_t a_NumShorts, AString & a_UTF8);

/** Converts a unicode character to its UTF8 representation. */
extern KEEN_API_EXPORT AString UnicodeCharToUtf8(unsigned a_UnicodeChar);

/** Converts a UTF-8 string into a UTF-16 BE string. */
extern KEEN_API_EXPORT std::u16string UTF8ToRawBEUTF16(const AString & a_String);

/** Creates a nicely formatted HEX dump of the given memory block.
Max a_BytesPerLine is 120. */
extern KEEN_API_EXPORT AString & CreateHexDump(AString & a_Out, const void * a_Data, size_t a_Size, size_t a_BytesPerLine);

/** Returns a copy of a_Message with all quotes and backslashes escaped by a backslash. */
extern KEEN_API_EXPORT AString EscapeString(const AString & a_Message); 

/** Removes all control codes used by MC for colors and styles. */
extern KEEN_API_EXPORT AString StripColorCodes(const AString & a_Message);

/** URL-Decodes the given string.
The first value specifies whether the decoding was successful.
The second value is the decoded string, if successful. */
extern KEEN_API_EXPORT std::pair<bool, AString> URLDecode(const AString & a_String);  // Exported to Lua as cUrlParser::UrlDecode()

/** URL-encodes the given string. */
extern KEEN_API_EXPORT AString URLEncode(const AString & a_Text);

/** Replaces all occurrences of char a_From inside a_String with char a_To. */
extern KEEN_API_EXPORT AString ReplaceAllCharOccurrences(const AString & a_String, char a_From, char a_To);  // Needn't export to Lua, since Lua doesn't have chars anyway

/** Decodes a Base64-encoded string into the raw data */
extern KEEN_API_EXPORT AString Base64Decode(const AString & a_Base64String);  // Exported manually due to embedded NULs and extra parameter

/** Encodes a string into Base64 */
extern KEEN_API_EXPORT AString Base64Encode(const AString & a_Input);  // Exported manually due to embedded NULs and extra parameter

/** Reads two bytes from the specified memory location and interprets them as BigEndian short */
extern KEEN_API_EXPORT short GetBEShort(const char * a_Mem);

/** Reads two bytes from the specified memory location and interprets them as BigEndian unsigned short */
extern KEEN_API_EXPORT unsigned short GetBEUShort(const char * a_Mem);

/** Reads four bytes from the specified memory location and interprets them as BigEndian int */
extern KEEN_API_EXPORT int GetBEInt(const char * a_Mem);

/** Writes four bytes to the specified memory location so that they interpret as BigEndian int */
extern KEEN_API_EXPORT void SetBEInt(char * a_Mem, Int32 a_Value);

/** Splits a string that has embedded \0 characters, on those characters.
a_Output is first cleared and then each separate string is pushed back into a_Output.
Returns true if there are at least two strings in a_Output (there was at least one \0 separator). */
extern KEEN_API_EXPORT bool SplitZeroTerminatedStrings(const AString & a_Strings, AStringVector & a_Output);

/** Merges the two vectors of strings, removing duplicate entries from the second vector.
The resulting vector contains items from a_Strings1 first, then from a_Strings2.
The order of items doesn't change, only the duplicates are removed.
If a_Strings1 contains duplicates, the result will still contain those duplicates. */
extern KEEN_API_EXPORT AStringVector MergeStringVectors(const AStringVector & a_Strings1, const AStringVector & a_Strings2);

/** Concatenates the specified strings into a single string, separated by the specified separator. */
extern KEEN_API_EXPORT AString StringsConcat(const AStringVector & a_Strings, char a_Separator);


extern KEEN_API_EXPORT AString generateUuid();

/** Parses any integer type. Checks bounds and returns errors out of band. */
template <class T>
bool StringToInteger(const AString & a_str, T & a_Num)
{
	size_t i = 0;
	bool positive = true;
	T result = 0;
	if (a_str[0] == '+')
	{
		i++;
	}
	else if (a_str[0] == '-')
	{
		i++;
		positive = false;
	}
	if (positive)
	{
		for (size_t size = a_str.size(); i < size; i++)
		{
			if ((a_str[i] < '0') || (a_str[i] > '9'))
			{
				return false;
			}
			if (std::numeric_limits<T>::max() / 10 < result)
			{
				return false;
			}
			result *= 10;
			T digit = static_cast<T>(a_str[i] - '0');
			if (std::numeric_limits<T>::max() - digit < result)
			{
				return false;
			}
			result += digit;
		}
	}
	else
	{
		// Unsigned result cannot be signed!
		if (!std::numeric_limits<T>::is_signed)
		{
			return false;
		}

		for (size_t size = a_str.size(); i < size; i++)
		{
			if ((a_str[i] < '0') || (a_str[i] > '9'))
			{
				return false;
			}
			if (std::numeric_limits<T>::min() / 10 > result)
			{
				return false;
			}
			result *= 10;
			T digit = static_cast<T>(a_str[i] - '0');
			if (std::numeric_limits<T>::min() + digit > result)
			{
				return false;
			}
			result -= digit;
		}
	}
	a_Num = result;
	return true;
}





/** Returns a number (of any integer type T) from a key-value string map.
Returns a_Default if the key is not present or the value is not a number representable in type T. */
template <typename T>
T GetStringMapInteger(const AStringMap & a_Map, const AString & a_Key, T a_Default)
{
	// Try to locate the key:
	auto itr = a_Map.find(a_Key);
	if (itr == a_Map.end())
	{
		return a_Default;
	}

	// Try to convert the value to a number:
	T res = a_Default;
	if (!StringToInteger<T>(itr->second, res))
	{
		return a_Default;
	}
	return res;
}
