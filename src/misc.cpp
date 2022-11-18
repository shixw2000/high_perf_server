#include<unistd.h>
#include<sys/syscall.h>
#include<sys/types.h>
#include<signal.h>
#include<pthread.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<time.h>
#include<stdlib.h>
#include<regex.h>
#include<stdarg.h>
#include"misc.h"
#include"globaltype.h"


int getTid() {
    int tid = 0;

    tid = syscall(SYS_gettid);
    return tid;
}

void sleepSec(int sec) {
    sleep(sec);
}

void maskSig(int sig) {
    sigset_t sets;

    sigemptyset(&sets);
    sigaddset(&sets, sig);

    pthread_sigmask(SIG_BLOCK, &sets, NULL);
}

void armSig(int sig, void (*fn)(int)) {
    struct sigaction act;

    memset(&act, 0, sizeof(act));
    sigemptyset(&act.sa_mask);

    act.sa_flags = SA_RESTART;
    act.sa_handler = fn;

    sigaction(sig, &act, NULL);
}

void getRand(Void* buf, Int32 len) {
    static Uint64 g_rand = 0;
    int fd = -1;
    int cnt = 0;

    if (0 < len) {
        fd = open("/dev/urandom", O_RDONLY|O_NONBLOCK);
        if (0 <= fd) {
            cnt = read(fd, buf, len);
            close(fd);
        }

        if (cnt != len) {
            --g_rand;
            
            memcpy(buf, &g_rand, len);
        }
    }
    
    return;
}

Uint64 getSysTime() {
    Uint64 tm = 0;
    Int32 ret = 0;
    struct timespec ts;

    ret = clock_gettime(CLOCK_REALTIME_COARSE, &ts);
    if (0 == ret) {
        mulTime(tm, ts.tv_sec, ts.tv_nsec / 1000000);
    } 

    return tm;
}

Uint32 getMonoTime() {
    Uint32 tm = 0;
    Int32 ret = 0;
    struct timespec ts;

    ret = clock_gettime(CLOCK_MONOTONIC_COARSE, &ts);
    if (0 == ret) {
        tm = (Uint32)ts.tv_sec;
    }

    return tm;
}


static const Char DEF_BUILD_VER[] = __BUILD_VER__;
static const Int32 MAX_LOG_FILE_SIZE = 0x2000000;
static const Int32 MAX_LOG_FILE_CNT = 3;
static const Int32 MAX_LOG_NAME_SIZE = 32;
static const Int32 MAX_LOG_CACHE_SIZE = 0x100000;
static const Int32 MAX_CACHE_INDEX = 8;
static Int32 g_log_level = 0;
static Bool g_log_stdin = TRUE;
static const Int32 MAX_LOG_LEVEL = 3;
static const Char* DEF_LOG_DESC[MAX_LOG_LEVEL] = {
    "ERROR", "INFO", "DEBUG" 
};

static FILE* g_log_hd = NULL;
static Byte g_cur_log_index = 0;
static const Char DEF_LOG_LINK_NAME[] = "agent.log";
static const Char DEF_NULL_FILE[] = "/dev/null";
static const Char DEF_LOG_NAME_PATTERN[] = "^agent_([[:digit:]]{3}).log$";
static Char g_buf[MAX_CACHE_INDEX][MAX_LOG_CACHE_SIZE];
static Byte g_buf_index = 0;


static Void findLogIndex() {
    Int32 ret = 0; 
    int index = 0;
    regex_t reg;
    regmatch_t matchs[2]; 
    Char szFile[MAX_LOG_NAME_SIZE] = {0};
    
    g_cur_log_index = 0;
    
    ret = readlink(DEF_LOG_LINK_NAME, szFile, MAX_LOG_NAME_SIZE-1);
    if (0 <= ret) {
        szFile[ret] = '\0';
        
        regcomp(&reg, DEF_LOG_NAME_PATTERN, REG_EXTENDED);
        ret = regexec(&reg, szFile, 2, matchs, 0);
        if (0 == ret) {
            index = atoi(&szFile[ matchs[1].rm_so ]);
        }
    } 

    g_cur_log_index = index;
}

static Void creatFileLog(const char mode[]) {
    const Char* psz = NULL;
    FILE* f = NULL;
    Int32 ret = 0;
    int index = 0;
    Char szFile[MAX_LOG_NAME_SIZE] = {0};
    
    index = ATOMIC_FETCH_INC(&g_cur_log_index);
    index %= MAX_LOG_FILE_CNT;
    snprintf(szFile, sizeof(szFile), "agent_%03d.log", index);

    psz = szFile;
    if (NULL != g_log_hd) {
        f = freopen(psz, mode, g_log_hd); 
        if (NULL == f) {
            psz = DEF_NULL_FILE;
            freopen(psz, mode, g_log_hd); 
        }
    } else {
        g_log_hd = fopen(psz, mode);
        if (NULL == g_log_hd) {
            psz = DEF_NULL_FILE;
            g_log_hd = fopen(psz, mode);
        }
    }
    
    ret = unlink(DEF_LOG_LINK_NAME);
    if (0 == ret || ENOENT == errno) {
        symlink(psz, DEF_LOG_LINK_NAME);
    } 
}

static Void statFileLog() {
    static Bool g_log_creating = FALSE;
    Int32 pos = 0; 

    pos = (Int32)ftell(g_log_hd);
    if (pos < MAX_LOG_FILE_SIZE || g_log_creating) {
        return;
    } else {
        g_log_creating = TRUE;
        
        creatFileLog("wb"); 

        g_log_creating = FALSE;
    }
}

Void setLogLevel(Int32 level) {
    if (MAX_LOG_LEVEL > level && 0 <= level) {
        g_log_level = level;
    } else {
        g_log_level = 0;
    }
}

Void setLogStdin(Bool yes) {
    g_log_stdin = yes;
}

Void initLog() {
    findLogIndex();
    creatFileLog("ab"); 
}

Void finishLog() {
    if (NULL != g_log_hd) {
        fclose(g_log_hd);
        g_log_hd = NULL;
    }
} 

Void formatLog(int level, const char format[], ...) { 
    int maxlen = 0;
    int size = 0;
    int cnt = 0;
    Char* psz = NULL;
    struct tm* result = NULL; 
    struct tm tm;
    time_t t = 0;
    va_list ap;
    Byte index = 0;

    if (level > g_log_level || NULL == g_log_hd) {
        return;
    }
    
    t = (time_t)to_sec(getSysTime());
    size = 0;
    maxlen = MAX_LOG_CACHE_SIZE-1;    
    
    result = localtime_r(&t, &tm); 
    if (NULL != result) { 
        index = ATOMIC_FETCH_INC(&g_buf_index); 
        index = index % MAX_CACHE_INDEX;
        psz = g_buf[index];
        
        cnt = snprintf(&psz[size], maxlen, "[%s]", DEF_LOG_DESC[level]);
        maxlen -= cnt;
        size += cnt;
        
        cnt = strftime(&psz[size], maxlen, "<%Y-%m-%d %H:%M:%S> ", &tm);
        maxlen -= cnt;
        size += cnt;
    
        va_start(ap, format);
        cnt = vsnprintf(&psz[size], maxlen, format, ap);
        va_end(ap); 
        if (0 <= cnt && cnt < maxlen) {
            maxlen -= cnt;
            size += cnt;
        } else if (cnt >= maxlen && 0 < maxlen) {
            cnt= maxlen - 1;
            maxlen -= cnt;
            size += cnt;
        }

        psz[size++] = '\n';

        fwrite(psz, 1, size, g_log_hd); 
        
        statFileLog(); 

        if (g_log_stdin) {
            fwrite(psz, 1, size, stdout); 
        }
    }
    
    return;
}


Void initLib() {
    srand(time(NULL));

    initLog();
}

Void finishLib() {
    LOG_INFO("prog_stop| version=%s| msg=exit now|", DEF_BUILD_VER);
    
    finishLog();
}

