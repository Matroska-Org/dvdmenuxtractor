#include <iostream>
#include "utilities.h"

QString Utilities::CreateUID()
{
	return QString("%1").arg( uint32_t(rand() | (rand() << 16)) );
}

QString Utilities::FormatTime(uint64_t a_time)
{
	int hour, min, sec;
	uint32_t nano;

	nano = a_time % 1000000000;
	a_time /= 1000000000;
	sec = a_time % 60;
	a_time /= 60;
	min = a_time % 60;
	hour = a_time / 60;

	return QString("%1:%2:%3.%4").arg(hour, 2, 10, QChar('0')).arg(min, 2, 10, QChar('0')).arg(sec, 2, 10, QChar('0')).arg(nano, 9, 10, QChar('0'));
}

QString Utilities::EncodeHex(const unsigned char *buffer, unsigned int size)
{
	QString result;

	for (unsigned i = 0; i < size; ++i)
		result += QString("%1 ").arg(buffer[i], 2, 16, QChar('0'));

	return result.trimmed();
}
