#ifndef __CONFIG_H__
#define __CONFIG_H__
#include<map>
#include<string>
#include"globaltype.h"
#include"listnode.h"
#include"datatype.h"


static const Int32 MAX_LINE_LEN = 128;
static const Int32 MAX_FILENAME_PATH_SIZE = 256;
static const Char DEF_AGENT_CONF_PATH[] = "service.conf";

static const Char DEF_SEC_GLOBAL_NAME[] = "global";
static const Char DEF_KEY_PASSWD_NAME[] = "password";
static const Char DEF_SYS_PASSWD[] = "123456";
static const Char DEF_KEY_LOG_LEVEL_NAME[] = "log_level";
static const Char DEF_KEY_LOG_STDIN_NAME[] = "log_stdin";

static const Char DEF_KEY_IS_REPLY_NAME[] = "is_reply";
static const Char DEF_KEY_PRNT_ORDER_NAME[] = "print_order";
static const Char DEF_KEY_FIRST_SND_CNT_NAME[] = "first_send_cnt";
static const Int32 MAX_PIN_PASSWD_SIZE = 64; 


struct Config { 
    list_head m_agent_list;
    Int32 m_log_level;
    Int32 m_log_stdin;
    Int32 m_reply;
    Int32 m_snd_cnt;
    Int32 m_prnt_order;
    Char m_passwd[MAX_PIN_PASSWD_SIZE];
};

class Parser {
    typedef std::string typeStr;
    typedef std::map<typeStr, typeStr> typeMap;
    typedef typeMap::iterator typeMapItr;
    typedef std::map<typeStr, typeMap> typeConf;
    typedef typeConf::iterator typeConfItr; 
    
public:
    Parser();
    ~Parser();

    Int32 init(const Char* path);
    Void finish();
    
    inline Config* getConf() {
        return &m_config;
    }

private:
    Int32 parse(const Char* path);
    Int32 parseAddr(TcpParam* param, Char* url);

    Int32 analyseGlobal();
    
    Int32 analyseAgent();

    Int32 analyseAddress(typeMap& imap, list_head* list);
    
    Void strip(Char* text);
    Void stripComment(Char* text);
    int getKeyVal(const Char* line, Char* key, Int32 key_len,
        Char* val, Int32 val_len);
    Bool getSection(Char line[]);

    Int32 getKeyStr(typeMap& imap, const Char key[],
        Char val[], Int32 maxlen);
    
    Int32 getKeyInt(typeMap& imap, const Char key[], Int32* val);

    Int32 parseAgentCli(Int32 type, typeMap& imap, list_head* list);
    Int32 parseAgentSrv(Int32 type, typeMap& imap, list_head* list);

    Void freeAgents(list_head* list);
    
private:
    Config m_config;
    typeConf m_conf; 
};

#endif

