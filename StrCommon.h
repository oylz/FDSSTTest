#ifndef _STRCOMMONH_
#define _STRCOMMONH_
#include <string>
#include <vector>
#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
#include <time.h>

static int gettimeofday(struct timeval *tp, void *tzp)
{
	time_t clock;
	struct tm tm;
	SYSTEMTIME wtm;

	GetLocalTime(&wtm);
	tm.tm_year = wtm.wYear - 1900;
	tm.tm_mon = wtm.wMonth - 1;
	tm.tm_mday = wtm.wDay;
	tm.tm_hour = wtm.wHour;
	tm.tm_min = wtm.wMinute;
	tm.tm_sec = wtm.wSecond;
	tm.tm_isdst = -1;
	clock = mktime(&tm);
	tp->tv_sec = clock;
	tp->tv_usec = wtm.wMilliseconds * 1000;

	return (0);
}
static void usleep(int64_t us) {
	int64_t s = us / 1000;
	Sleep(s);
}
#else
#include <sys/time.h>
#endif

using namespace cv;


static int64_t gtm() {
	struct timeval tm;
	gettimeofday(&tm, 0);
	int64_t re = ((int64_t)tm.tv_sec) * 1000 * 1000 + tm.tv_usec;
	return re;
}



#endif
