#include "include/bitcoin-structs.h"
#include <string.h>
// Encode an integer as a varint.
size_t VarintEncode(U32 value, U8 *output)
{
	size_t i = 0;
	while (value >= 0x80)
	{
		output[i++] = (value & 0x7F) | 0x80;  // Set the continuation bit
		value >>= 7;
	}
	output[i++] = value;  // Last byte, without the continuation bit
	return i;
}


// Decode a varint into an integer.
size_t VarintDecode(const U8 *input, U32 *output)
{
	U32 result = 0;
	size_t shift = 0;
	size_t i = 0;

	while (input[i] & 0x80)
	{
		result |= (input[i] & 0x7F) << shift;
		shift += 7;
		i++;
	}
	result |= input[i] << shift;
	i++;					// Increment to account for the last byte read
	if (output != NULL)
	{
		*output = result;
	}
	return i;
}








void PrintByteString(const U8 *bytes, size_t size, FILE *output)
{
	for (size_t i = 0; i < size; ++i)
	{
		fprintf(output, "%02X", bytes[i]);
	}
}


// Function to convert a U32 value to an array of 4 U8 values
void ConvertUint32ToUint8Array(U32 value, U8 *outputArray)
{
	outputArray[0] = (value >> 24) & 0xFF;  // Most significant byte
	outputArray[1] = (value >> 16) & 0xFF;  // Second byte
	outputArray[2] = (value >> 8)  & 0xFF;  // Third byte
	outputArray[3] = (value)       & 0xFF;  // Least significant byte
}


// Function to encode a custom Bitcoin-style varint
// Returns the number of bytes used
size_t EncodeVarint128(U64 value, U8* data)
{
	U8 buffer[10];
	int i = 0;

	while (value > 0x7F) {
		buffer[i++] = (value & 0x7F) | 0x80;
		value = (value >> 7) - 1;
	}
	buffer[i++] = (U8)(value & 0x7F);

	// Write the buffer in reverse order and count the number of bytes written
	for (int j = i - 1; j >= 0; j--)
	{
		*data++ = buffer[j];
	}
	return i;								// Returns the number of bytes used to encode the varint
}

size_t DecodeVarint128(const void *data, U64 *value)
{
	const U8 *ret	= (const U8 *)data;		// Cast the void pointer to a single byte pointer
	const U8 *start	= ret;					// Remember the start position to calculate the number of bytes read

	*value = 0;								// Set the return value to zero initially
	U8 byte = 0;							// Each byte we are streaming in
	do {
		byte = *ret++;						// Get the byte value and or the 7 lower bits into the result.
		*value = (*value << 7) | (U64)(byte & 0x7F);
		// If this is not the last byte, we increment the value we are accumulating by one.
		// This is extremely non-standard and I could not find a reference to this technique
		// anywhere online other than the bitcoin source code itself.

		if (byte & 0x80)
		{
			(*value)++;
		}
	} while (byte & 0x80);

	return ret - start; // Return the number of bytes read
}

U32 ChangeEndiannessUint32(U32 value)
{
	return ((value >> 24) & 0x000000FF)	|	// Move byte 3 to byte 0
	((value >> 8)  & 0x0000FF00)		|	// Move byte 2 to byte 1
	((value << 8)  & 0x00FF0000)		|	// Move byte 1 to byte 2
	((value << 24) & 0xFF000000);			// Move byte 0 to byte 3
}



// WARN: CompactSize functions are untested yet, if there's a bug it probably comes from here.

// Function to encode a number into a compact size field
size_t CompactSizeEncode(U64 value, U8* output)
{
	if (value <= 252)
	{
		// Single-byte value
		output[0] = (U8)value;
		return 1; // 1 byte written
	}
	else if (value <= 0xFFFF)
	{
		// FD prefix with 2 bytes
		output[0] = 0xFD;
		output[1] = (U8)(value & 0xFF);
		output[2] = (U8)((value >> 8) & 0xFF);
		return 3; // 3 bytes written
	}
	else if (value <= 0xFFFFFFFF)
	{
		// FE prefix with 4 bytes
		output[0] = 0xFE;
		output[1] = (U8)(value & 0xFF);
		output[2] = (U8)((value >> 8) & 0xFF);
		output[3] = (U8)((value >> 16) & 0xFF);
		output[4] = (U8)((value >> 24) & 0xFF);
		return 5; // 5 bytes written
	}
	else
{
		// FF prefix with 8 bytes
		output[0] = 0xFF;
		output[1] = (U8)(value & 0xFF);
		output[2] = (U8)((value >> 8) & 0xFF);
		output[3] = (U8)((value >> 16) & 0xFF);
		output[4] = (U8)((value >> 24) & 0xFF);
		output[5] = (U8)((value >> 32) & 0xFF);
		output[6] = (U8)((value >> 40) & 0xFF);
		output[7] = (U8)((value >> 48) & 0xFF);
		output[8] = (U8)((value >> 56) & 0xFF);
		return 9; // 9 bytes written
	}
}



size_t CompactSizeDecode(const U8 *data, size_t dataSize, U64 *value)
{
	if (data == NULL || value == NULL || dataSize == 0)
	{
		if (value)
		{
			memcpy(value, &(U64){0}, sizeof(U64));
		}
		return 0;
	}

	U8		first	= data[0];
	U64		tmp		= 0;
	size_t	ret		= 0;

	if (first <= 0xFC)
	{
		tmp = first;
		ret = 1;
	}
	else if (first == 0xFD)
	{
		if (dataSize < 3)
		{
			tmp = 0; return dataSize;
		}
		tmp =	((U64)data[1])	|
				((U64)data[2] << 8);
		ret = 3;
	}
	else if (first == 0xFE)
	{
		if (dataSize < 5)
		{
			tmp = 0;
			return dataSize;
		}
		tmp =	((U64)data[1])			|
				((U64)data[2] <<  8)	|
				((U64)data[3] << 16)	|
				((U64)data[4] << 24);
		ret = 5;
	}
	else /* first == 0xFF */
	{
		if (dataSize < 9)
		{
			tmp = 0;
			return dataSize;
		}
		tmp =	((U64)data[1])			|
				((U64)data[2] <<  8)	|
				((U64)data[3] << 16)	|
				((U64)data[4] << 24)	|
				((U64)data[5] << 32)	|
				((U64)data[6] << 40)	|
				((U64)data[7] << 48)	|
				((U64)data[8] << 56);
		ret = 9;
	}

	// Safe even if 'value' is at an odd address:
	memcpy(value, &tmp, sizeof tmp);
	return ret;
}


