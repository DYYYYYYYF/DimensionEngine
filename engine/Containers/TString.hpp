#include "string.h"

char* Strtrim(char* str) {

	while (isspace((unsigned char)*str)) {
		str++;
	}

	if (*str) {
		char* p = str;
		while (*p) {
			p++;
		}

		while (isspace((unsigned char)*(--p)))
			;
		p[1] = '\0';
	}

	return str;
}

int StringIndexOf(char* str, char c) {
	if (!str) {
		return -1;
	}

	size_t Length = strlen(str);
	if (Length > 0) {
		for (int i = 0; i < Length; ++i) {
			if (str[i] == c) {
				return i;
			}
		}
	}

	return -1;
}

void StringMid(char* dst, const char* src, size_t start, int length = -1) {
	if (length == 0) {
		return;
	}

	size_t SrcLength = strlen(src);
	if (start >= SrcLength) {
		dst[0] = 0;
		return;
	}

	if (length > 0) {
		for (size_t i = start, j = 0; j < length && src[i]; ++i, ++j) {
			dst[j] = src[i];
		}
		dst[start + length] = 0;
	}
	else {
		// If a negative value is passed, proceed to the end of string.
		size_t j = 0;
		for (size_t i = start; src[i]; ++i, ++j) {
			dst[j] = src[i];
		}

		dst[start + j] = 0;
	}
}