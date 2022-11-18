#ifndef __MISC_H__
#define __MISC_H__
#include"sysoper.h"


#define LOG_DEBUG(format,args...)  formatLog(2, format, ##args)
#define LOG_INFO(format,args...)  formatLog(1, format, ##args)
#define LOG_ERROR(format,args...)  formatLog(0, format, ##args)

extern Void initLib();
extern Void finishLib();

extern Void formatLog(int level, const char format[], ...);
extern Void setLogLevel(Int32 level);
extern Void setLogStdin(Bool yes);

Int32 getTid();
Void maskSig(int sig);
Void armSig(int sig, void (*)(int));
Void sleepSec(int sec);
Void getRand(void* buf, int len);

Uint64 getSysTime();
Uint32 getMonoTime();


#endif

