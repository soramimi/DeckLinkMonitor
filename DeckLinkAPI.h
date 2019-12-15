#ifndef DECKLINKAPI_H
#define DECKLINKAPI_H

#include "DeckLinkAPI.h"
#include <QMetaType>
#include <QString>

#if defined(Q_OS_WIN)
#include <Windows.h>
#include "sdk/Win/DeckLinkAPI_h.h"
typedef BOOL dlbool_t;
typedef IID CFUUIDBytes;


#elif defined(Q_OS_MACX)
#include "sdk/Mac/include/DeckLinkAPI.h"
typedef bool dlbool_t;
typedef CFStringRef BSTR;

static inline QString toQString(CFStringRef str)
{
	if (!str) return QString();
	CFIndex length = CFStringGetLength(str);
	if (length == 0) return QString();
	QString string(length, Qt::Uninitialized);
	CFStringGetCharacters(str, CFRangeMake(0, length), reinterpret_cast<UniChar *>(const_cast<QChar *>(string.unicode())));
	return string;
}

#else

#include "sdk/Linux/include/DeckLinkAPI.h"
typedef bool dlbool_t;
#endif

class DLString {
private:
#ifdef Q_OS_WIN
	BSTR str = nullptr;
#else
	char const *str = nullptr;
#endif
public:
	~DLString();
	void clear();
#ifdef Q_OS_WIN
	BSTR *operator & ()
	{
		return &str;
	}
	operator QString ()
	{
		return str ? QString::fromUtf16((ushort const *)str) : QString();
	}
	operator std::string ()
	{
		return operator QString ().toStdString();
	}
#else
	char const **operator & ()
	{
		return &str;
	}
	operator QString ()
	{
		return str ? QString::fromUtf8(str) : QString();
	}
	operator std::string ()
	{
		return str ? std::string(str) : std::string();
	}
#endif
};

HRESULT GetDeckLinkIterator(IDeckLinkIterator **deckLinkIterator);

#endif // DECKLINKAPI_H
