#include "DeckLinkAPI.h"

DLString::~DLString()
{
	clear();
}

void DLString::clear()
{
	if (str) {
#ifdef Q_OS_WIN
		SysFreeString(str);
#else
		DeleteString(str);
#endif
		str = nullptr;
	}
}
