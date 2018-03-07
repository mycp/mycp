/*
*****************************************************
*
* file        : pstr.h
* description : pstr
* author      : yhz
* version     : 1.0
* date        : 2010-08-30
*
*****************************************************
*/

#ifndef __pstr_h__
#define __pstr_h__

#define STR_NULL	(const char*)0
#define WCS_NULL	(const wchar_t*)0

static void pmemset(void * _Dst, int _Val, size_t _Size);

static size_t pstrlen(const char * str);
static size_t pstrlen(const wchar_t * str);
template <typename T>
static size_t pstrlen(const T * str);

static size_t pstrcpy(char * des, const char * src);
static size_t pstrcpy(wchar_t * des, const wchar_t * src);
template <typename T>
static size_t pstrcpy(T * des, const T * src);

static void plstrcpy(char * des, const char * src, size_t len);
static void plstrcpy(wchar_t * des, const wchar_t * src, size_t len);
template <typename T>
static void plstrcpy(T * des, const T * src, size_t len);

static int pstrcmp(const char * src, const char * compare);
static int pstrcmp(const wchar_t * src, const wchar_t * compare);
template <typename T>
static int pstrcmp(const T * src, const T * compare);
template <typename T>
static int plstrcmp(const T * src, const T * compare, size_t len);

template <typename T>
static const T * plstrstr(const T * sourceBuffer, const T * findBuffer, size_t sourceSize, size_t fineSize);

template <typename T>
static const T * plstrstr_r(const T * sourceBuffer, const T * findBuffer, size_t sourceSize, size_t fineSize);

template <typename T>
static bool pstrcompare(const T * src, const T * compare);


template <typename T>
const T * pstrstrl(const T * sourceBuffer, const T * findBuffer, size_t sourceSize, size_t fineSize);

template <typename T>
static size_t pgetstr(T * outBuffer, size_t bufferSize, const T * src, T endCh, int* pOutFindIndex=NULL);	// pOutFindIndex 1=start
//static size_t pgetstr(T * outBuffer, size_t bufferSize, const T * src, T endCh);

template <typename T>
static size_t psetstr(T * outBuffer, size_t bufferSize, const T * src, T findCh, T addCh);

template <typename T>
static size_t pstrtrim_left(T * inOutBuffer);

template <typename T>
static size_t pstrtrim_right(T * inOutBuffer);

template <typename T>
static size_t pstrtrim_left(T * inOutBuffer, T trimChar);

template <typename T>
static size_t pstrtrim_right(T * inOutBuffer, T trimChar);


void pmemset(void * _Dst, int _Val, size_t _Size)
{
	char * dsttemp = (char*)_Dst;
	while (_Size > 0)
		dsttemp[--_Size] = _Val;
}

size_t pstrlen(const char * str)
{
	size_t i = 0;
	size_t result = 0;
	if (str == 0) return 0;
	while (str[i++] != '\0')
		++result;
	return result;
}

size_t pstrlen(const wchar_t * str)
{
	size_t i = 0;
	size_t result = 0;
	if (str == 0) return 0;
	while (str[i++] != L'\0')
		++result;
	return result;
}

template <typename T>
size_t pstrlen(const T * str)
{
	size_t i = 0;
	size_t result = 0;
	T tnull = sizeof(T) == 1 ? '\0' : L'\0';
	if (str == 0) return 0;
	while (str[i++] != tnull)
		++result;
	return result;
}

size_t pstrcpy(char * des, const char * src)
{
	size_t i = 0;
	while (src[i] != '\0')
		des[i] = src[i++];
	if (des[i] != '\0')
		des[i] = '\0';
	return i;
}

size_t pstrcpy(wchar_t * des, const wchar_t * src)
{
	size_t i = 0;
	while (src[i] != L'\0')
		des[i] = src[i++];
	if (des[i] != L'\0')
		des[i] = L'\0';
	return i;
}

template <typename T>
size_t pstrcpy(T * des, const T * src)
{
	size_t i = 0;
	T tnull = sizeof(T) == 1 ? '\0' : L'\0';
	while (src[i] != tnull)
		des[i] = src[i++];
	des[i] = tnull;
	return i;
}

void plstrcpy(char * des, const char * src, size_t len)
{
	size_t i = 0;
	while (i < len)
		des[i] = src[i++];
	des[i] = '\0';
}

void plstrcpy(wchar_t * des, const wchar_t * src, size_t len)
{
	size_t i = 0;
	while (i < len)
		des[i] = src[i++];
	des[i] = L'\0';
}

template <typename T>
void plstrcpy(T * des, const T * src, size_t len)
{
	size_t i = 0;
	T tnull = sizeof(T) == 1 ? '\0' : L'\0';
	while (i < len)
		des[i] = src[i++];
	des[i] = tnull;
}

int pstrcmp(const char * src, const char * compare)
{
	size_t i = 0;
	if (src == STR_NULL)
		return compare == STR_NULL ? 0 : -1;
	if (compare == STR_NULL)
		return src == STR_NULL ? 0 : 1;

	while (1)
	{
		if (src[i] == '\0')
			return compare[i] == '\0' ? 0 : -1;
		if (compare[i] == '\0')
			return src[i] == '\0' ? 0 : 1;

		if (src[i] > compare[i])
			return 1;
		if (src[i] < compare[i])
			return -1;
		++i;
	}

	return 0;
}

int pstrcmp(const wchar_t * src, const wchar_t * compare)
{
	size_t i = 0;
	if (src == WCS_NULL)
		return compare == WCS_NULL ? 0 : -1;
	if (compare == WCS_NULL)
		return src == WCS_NULL ? 0 : 1;

	while (1)
	{
		if (src[i] == L'\0')
			return compare[i] == L'\0' ? 0 : -1;
		if (compare[i] == L'\0')
			return src[i] == L'\0' ? 0 : 1;

		if (src[i] > compare[i])
			return 1;
		if (src[i] < compare[i])
			return -1;
		++i;
	}

	return 0;
}

template <typename T>
int pstrcmp(const T * src, const T * compare)
{
	size_t i = 0;
	if (src == (T*)0)
		return compare == (T*)0 ? 0 : -1;
	if (compare == (T*)0)
		return src == (T*)0 ? 0 : 1;

	T tnull = sizeof(T) == 1 ? '\0' : L'\0';
	while (1)
	{
		if (src[i] == tnull)
			return compare[i] == tnull ? 0 : -1;
		if (compare[i] == tnull)
			return src[i] == tnull ? 0 : 1;

		if (src[i] > compare[i])
			return 1;
		if (src[i] < compare[i])
			return -1;
		++i;
	}

	return 0;
}
template <typename T>
int plstrcmp(const T * src, const T * compare, size_t len)
{
	size_t i = 0;
	if (src == (T*)0)
		return compare == (T*)0 ? 0 : -1;
	if (compare == (T*)0)
		return src == (T*)0 ? 0 : 1;

	T tnull = sizeof(T) == 1 ? '\0' : L'\0';
	while (i < len)
	{
		if (src[i] == tnull)
			return compare[i] == tnull ? 0 : -1;
		if (compare[i] == tnull)
			return src[i] == tnull ? 0 : 1;

		if (src[i] > compare[i])
			return 1;
		if (src[i] < compare[i])
			return -1;
		++i;
	}

	return 0;
}

template <typename T>
const T * plstrstr(const T * sourceBuffer, const T * findBuffer, size_t sourceSize, size_t fineSize)
{
	if (sourceBuffer == NULL || findBuffer == NULL) return (T*)0;
	if (fineSize == 0 || fineSize > sourceSize) return (T*)0;

	size_t sourceIndex = 0;
	size_t findIndex = 0;
	while (sourceIndex < sourceSize)
	{
		if (sourceBuffer[sourceIndex] != findBuffer[findIndex])
		{
			if (findIndex == 0)
				sourceIndex++;
			else
				findIndex = 0;
			continue;
		}
		sourceIndex++;
		if (++findIndex == fineSize)
			return sourceBuffer + sourceIndex - findIndex;
	}

	return (T*)0;
}

template <typename T>
const T * plstrstr_r(const T * sourceBuffer, const T * findBuffer, size_t sourceSize, size_t fineSize)
{
	if (sourceBuffer == NULL || findBuffer == NULL) return (T*)0;
	if (fineSize == 0 || fineSize > sourceSize) return (T*)0;

	size_t sourceIndex = sourceSize;
	size_t findIndex = fineSize;
	while ((sourceIndex--) > 0)
	{
		if (sourceBuffer[sourceIndex] != findBuffer[--findIndex])
		{
			if (findIndex > 0)
				findIndex = fineSize;
			continue;
		}
		if (findIndex == 0)
			return sourceBuffer + sourceIndex;
	}

	return (T*)0;
}

template <typename T>
bool pstrcompare(const T * pBuffer, const T * pCompare)
{
	int i1 = 0, i2 = 0;
	T tnull = sizeof(T) == 1 ? '\0' : L'\0';
	while (pCompare[i2] != tnull)
	{
		if (pCompare[i2++] != pBuffer[i1] || tnull == pBuffer[i1])
		{
			return false;
		}
		i1++;
	}
	return true;
}

template <typename T>
const T * pstrstrl(const T * sourceBuffer, const T * findBuffer, size_t sourceSize, size_t fineSize)
{
	if (sourceBuffer == NULL || findBuffer == NULL) return NULL;
	if (fineSize > sourceSize) return NULL;

	size_t sourceIndex = 0;
	size_t findIndex = 0;
	while (sourceIndex < sourceSize)
	{
		if ((T)sourceBuffer[sourceIndex] != (T)findBuffer[findIndex])
		{
			if (findIndex == 0)
				sourceIndex++;
			else
				findIndex = 0;
			continue;
		}
		sourceIndex++;
		if (++findIndex == fineSize)
			return sourceBuffer + sourceIndex - findIndex;
	}
	return NULL;
}

template <typename T>
size_t pgetstr(T * outBuffer, size_t bufferSize, const T * src, T endCh, int* pOutFindIndex)
{
	// *增加支持遇到 '\'' 字符串，继续处理
	size_t result = 0;
	size_t srcIndex = 0;
	T tnull = sizeof(T) == 1 ? '\0' : L'\0';
	int nFindStringHead = 0;
	int nFindStringEnd = 0;

	if (pOutFindIndex!=0)
		*pOutFindIndex = -1;
	int nTranCodeCount = 0;
	while (result < bufferSize && src[srcIndex] != tnull)
	{
		if (src[srcIndex]=='\'')
		{
			if (nFindStringHead==0)
			{
				// 找到第一个 '
				nFindStringHead = 1;
			}else if (outBuffer[result-1] == '\\')
			{
				// 第N个 ' 被转久
				outBuffer[--result] = endCh;
				srcIndex++;
				nTranCodeCount++;
				continue;
			}else
			{
				nFindStringEnd = 1;
			}
		}
		if (src[srcIndex] == endCh)
		{
			if (nFindStringHead==1 && nFindStringEnd==0)
			{
				// ** 找到未结束字符串，继续处理
				outBuffer[result++] = src[srcIndex++];
				continue;
			}
			if (result==0 || outBuffer[result-1] != '\\')
			{
				if (result==0)
					nTranCodeCount = 1;
				break;
			}
			outBuffer[result-1] = endCh;
			srcIndex++;
			continue;
		}
		if (nTranCodeCount==0)
			nTranCodeCount = 1;	// 至少标识为1，最后汇总计算才会正确
		outBuffer[result++] = src[srcIndex++];
	}
	outBuffer[result] = tnull;
	if (pOutFindIndex!=0)
		*pOutFindIndex = (result+nTranCodeCount);
	return result;
}

template <typename T>
size_t psetstr(T * outBuffer, size_t bufferSize, const T * src, T findCh, T addCh)
{
	size_t result = 0;
	size_t srcIndex = 0;
	T tnull = sizeof(T) == 1 ? '\0' : L'\0';
	while (result < bufferSize && src[srcIndex] != tnull)
	{
		if (src[srcIndex] == findCh)
		{
			outBuffer[result++] = addCh;
			if (result == bufferSize)
				break;
		}
		outBuffer[result++] = src[srcIndex++];
	}
	outBuffer[result] = tnull;
	return result;
}

template <typename T>
size_t pstrtrim_left(T * inOutBuffer)
{
	size_t spaceCount = 0;
	size_t result = pstrlen<T>(inOutBuffer);
	T tnull = sizeof(T) == 1 ? '\0' : L'\0';
	T tspace = sizeof(T) == 1 ? ' ' : L' ';
	T ttab = sizeof(T) == 1 ? '\t' : L'\t';
	while (spaceCount < result && (inOutBuffer[spaceCount] == tspace || inOutBuffer[spaceCount] == ttab))
	{
		spaceCount++;
	}
	if (spaceCount > 0)
	{
		result -= spaceCount;
		for (size_t i=0; i<result; i++)
			inOutBuffer[i] = inOutBuffer[i+spaceCount];
		inOutBuffer[result] = tnull;
	}
	return result;
}

template <typename T>
size_t pstrtrim_right(T * inOutBuffer)
{
	size_t result = pstrlen<T>(inOutBuffer);
	T tnull = sizeof(T) == 1 ? '\0' : L'\0';
	T tspace = sizeof(T) == 1 ? ' ' : L' ';
	T ttab = sizeof(T) == 1 ? '\t' : L'\t';
	while (result>0 && inOutBuffer[result-1] == tspace || inOutBuffer[result-1] == ttab)
	{
		inOutBuffer[--result] = tnull;
	}
	return result;
}

template <typename T>
size_t pstrtrim_left(T * inOutBuffer, T trimChar)
{
	size_t trimCount = 0;
	size_t result = pstrlen<T>(inOutBuffer);
	T tnull = sizeof(T) == 1 ? '\0' : L'\0';
	while (trimCount < result && (inOutBuffer[trimCount] == trimChar))
	{
		trimCount++;
	}
	if (trimCount > 0)
	{
		result -= trimCount;
		for (size_t i=0; i<result; i++)
			inOutBuffer[i] = inOutBuffer[i+trimCount];
		inOutBuffer[result] = tnull;
	}
	return result;
}

template <typename T>
size_t pstrtrim_right(T * inOutBuffer, T trimChar)
{
	size_t result = pstrlen<T>(inOutBuffer);
	T tnull = sizeof(T) == 1 ? '\0' : L'\0';
	while (inOutBuffer[result-1] == trimChar)
	{
		inOutBuffer[--result] = tnull;
	}
	return result;
}

#endif //
