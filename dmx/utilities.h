#ifndef UTILITIES_H
#define UTILITIES_H

#include <QString>
#include <stdint.h>
#include "dmx_project.h"

#define T(x) (x)

namespace Utilities
{
	static const QString APPLICATION_NAME    ("DVDMenuXtractor: DMX - a fair-use tool");
	static const QString APPLICATION_VERSION PROJECT_VERSION;

	QString CreateUID();
	QString FormatTime(uint64_t a_time);
	QString EncodeHex(const unsigned char *buffer, unsigned size);
}

#undef T

#endif // UTILITIES_H
