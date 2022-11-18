#include<regex.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"config.h"
#include"datatype.h"
#include"sockutil.h"


Parser::Parser() {
    memset(&m_config, 0, sizeof(m_config));

    INIT_LIST_HEAD(&m_config.m_agent_list);
}

Parser::~Parser() {
}

Int32 Parser::init(const Char* path) {
    Int32 ret = 0;

    if (NULL == path) {
        path = DEF_AGENT_CONF_PATH;
    }
    
    ret = parse(path);
    if (0 != ret) {
        LOG_ERROR("parse_config| ret=%d| path=%s| msg=parse error|",
            ret, path);
        
        return ret;
    }

    ret = analyseGlobal();
    if (0 != ret) {
        LOG_ERROR("analyse_global| ret=%d| path=%s| msg=parse error|",
            ret, path);
        
        return ret;
    } 

    return ret;
}

Void Parser::finish() {
    freeAgents(&m_config.m_agent_list);
}

Void Parser::stripComment(Char* text) {
    Char* pch = NULL;

    pch = strchr(text, '#');
    if (NULL != pch) {
        *pch = '\0';
    }
}

Void Parser::strip(Char* text) {
    Int32 len = 0;
    Char* beg = NULL;
    Char* end = NULL;

    len = strnlen(text, MAX_LINE_LEN-1);
    beg = text;
    end = text + len;

    while (isspace(*beg) && beg < end) {
        ++beg;
    }

    while (beg < end && isspace(*(end-1))) {
        --end;
    }

    if (beg < end) {
        len = end - beg;
        memmove(text, beg, len);
        text[len] = '\0';
    } else {
        text[0] = '\0';
    }
}

int Parser::getKeyVal(const Char* line, Char* key, Int32 key_len,
    Char* val, Int32 val_len) {
    static const Char DEF_STR_PATTERN[] = 
        "^[[:space:]]*([_[:alnum:]]*)[[:space:]]*=[[:space:]]*\"(.*)\"[[:space:]]*$"; 
    int ret = 0;
    int len = 0;
    regex_t reg;
    regmatch_t matchs[3];  
    
    ret = regcomp(&reg, DEF_STR_PATTERN, REG_EXTENDED | REG_NEWLINE);
    if (0 != ret) {
        return -1;
    }

    do {
        ret = regexec(&reg, line, 3, matchs, 0);
        if (0 != ret) { 
            break;
        }
        
        len = (int)(matchs[1].rm_eo-matchs[1].rm_so);
        if (len < key_len) {
            
            memcpy(key, &line[ matchs[1].rm_so ], len);
            key[len] = '\0';
        } else {
            ret = -1;
            break;
        }

        len = (int)(matchs[2].rm_eo-matchs[2].rm_so);
        if (len < val_len) {            
            memcpy(val, &line[ matchs[2].rm_so ], len);
            val[len] = '\0';
        } else {
            ret = -1;
            break;
        }

        ret = 0;
    } while (0);

    regfree(&reg);
    return ret;
}

Bool Parser::getSection(Char line[]) {
    Int32 len = 0;

    len = strnlen(line, MAX_LINE_LEN);
    if (2 < len) {
        if ('[' == line[0] && ']' == line[len-1]) {
            len -= 2;
            memmove(line, &line[1], len);
            line[len] = '\0';
            
            return TRUE;
        } else {
            return FALSE;
        }
    } else {
        return FALSE;
    }
}

Int32 Parser::parse(const Char* path) {
    Int32 ret = 0;
    Bool bSec = TRUE;
    FILE* hd = NULL;
    Char line[MAX_LINE_LEN] = {0};
    Char key[MAX_LINE_LEN] = {0};
    Char val[MAX_LINE_LEN] = {0};
    Char* psz = NULL;
    typeConfItr itrConf;

    m_conf.clear();

    hd = fopen(path, "rb");
    if (NULL != hd) { 
        itrConf = m_conf.end();
        
        psz = fgets(line, MAX_LINE_LEN, hd);
        while (NULL != psz) {
            stripComment(line);
            strip(line);

            /* ignore empty or comment line */
            if ('#' != line[0] && '\0' != line[0]) {
                bSec = getSection(line);
                if (!bSec) {
                    if (itrConf != m_conf.end()) {
                        ret = getKeyVal(line, key, MAX_LINE_LEN, val, MAX_LINE_LEN);
                        if (0 == ret) {
                            typeMap& imap = itrConf->second;

                            imap[key] = val;
                        } else {
                            /* invalid item */
                            ret = -2;
                            break;
                        }
                    } else {
                        /* outside of a section */
                        ret = -3;
                        break;
                    }
                } else {
                    itrConf = m_conf.find(line);
                    if (itrConf == m_conf.end()) {
                        m_conf[line] = typeMap();
                        itrConf = m_conf.find(line);
                    } else {
                        /* duplicated section */
                        ret = -4;
                        break;
                    }
                }
            }
            
            psz = fgets(line, MAX_LINE_LEN, hd);
        }

        fclose(hd);
    } else {
        ret = -1;
    }

    return ret;
}

Int32 Parser::parseAddr(TcpParam* param, Char* url) {
    Int32 ret = 0;
    Int32 port = 0;
    char* psz = NULL;
    
    psz = strrchr(url, ':');
    if (NULL != psz) {
        *psz++ = '\0';
        port = atoi(psz);

        ret = buildParam(url, port, param);
        return ret;
    } else {
        return -1;
    }
}

Int32 Parser::analyseGlobal() {
    Int32 ret = 0;
    typeConfItr itrConf;
    typeMapItr itrKey;

    itrConf = m_conf.find(DEF_SEC_GLOBAL_NAME);
    if (m_conf.end() == itrConf) {
        return -1;
    }

    do {
        typeMap& imap = itrConf->second;
        
        ret = getKeyStr(imap, DEF_KEY_PASSWD_NAME, m_config.m_passwd, 
            MAX_PIN_PASSWD_SIZE);
        if (0 != ret) {
            LOG_ERROR("analyse_global| key=%s| msg=invalid|",
                DEF_KEY_PASSWD_NAME);
            break;
        } 

        ret = getKeyInt(imap, DEF_KEY_LOG_LEVEL_NAME, &m_config.m_log_level);
        if (0 != ret) {
            LOG_ERROR("analyse_global| key=%s| msg=invalid|",
                DEF_KEY_LOG_LEVEL_NAME);
            break;
        } 

        ret = getKeyInt(imap, DEF_KEY_LOG_STDIN_NAME, &m_config.m_log_stdin);
        if (0 != ret) {
            LOG_ERROR("analyse_global| key=%s| msg=invalid|",
                DEF_KEY_LOG_STDIN_NAME);
            break;
        } 

        ret = getKeyInt(imap, DEF_KEY_IS_REPLY_NAME, &m_config.m_reply);
        if (0 != ret) {
            LOG_ERROR("analyse_global| key=%s| msg=invalid|",
                DEF_KEY_IS_REPLY_NAME);
            break;
        }

        ret = getKeyInt(imap, DEF_KEY_PRNT_ORDER_NAME, &m_config.m_prnt_order);
        if (0 != ret) {
            LOG_ERROR("analyse_global| key=%s| msg=invalid|",
                DEF_KEY_PRNT_ORDER_NAME);
            break;
        }

        ret = getKeyInt(imap, DEF_KEY_FIRST_SND_CNT_NAME, &m_config.m_snd_cnt);
        if (0 != ret) {
            m_config.m_snd_cnt = 1;
            
            LOG_DEBUG("analyse_global| key=%s| msg=use defaut val:1|",
                DEF_KEY_FIRST_SND_CNT_NAME);
            
            ret = 0;
        }
    } while (0);

    return ret;
}

Int32 Parser::getKeyStr(typeMap& imap, const Char key[],
    Char val[], Int32 maxlen) {
    Int32 size = 0;
    typeMapItr itrKey;

    itrKey = imap.find(key);
    if (imap.end() != itrKey) {
        const typeStr& str = itrKey->second;

        size = (Int32)str.size();
        if (size < maxlen) {
            str.copy(val, size);
            val[size] = '\0';

            return 0;
        } else {
            return -1;
        }
    } else {
        return -1;
    } 
}

Int32 Parser::getKeyInt(typeMap& imap, const Char key[], Int32* val) {
    Int32 ret = 0;
    Char buf[MAX_LINE_LEN] = {0};

    ret = getKeyStr(imap, key, buf, MAX_LINE_LEN);
    if (0 == ret) {
        if (isdigit(buf[0])) {
            *val = atoi(buf);
            return 0;
        } else {
            return -1;
        }
    } else {
        return -1;
    } 
}

Void Parser::freeAgents(list_head* list) {
    list_node* pos = NULL;
    list_node* n = NULL;
    
    list_for_each_safe(pos, n, list) {
        list_del(pos, list);        
    }
}

