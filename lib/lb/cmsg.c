#include <string.h>
#include "cmsg.h"
#include "utils.h"

// decodes messages which have no variables in them
bool decode_fixed(const char* buff, CMsg* res) {
    const char* strs[] = {"execute", "round", "h?", "l?", "s?", "yourturn", 
            "game_over", "yes", "no"};
    const MType t[] = {EXECUTE_PHASE, ROUND, CHOOSE_H, CHOOSE_LONG, CHOOSE_SHORT, TURN, 
            GAME_OVER, ACK, NACK};
    const int numVars = 9;
    for (int i = 0; i < numVars; ++i) {
        if (!strcmp(buff, strs[i])) {
            res->type = t[i];
            return true;
        }
    }
    return false;
}

// read a line of text from f and decode into a message struct
CMsg decode_message(FILE* f)
{
    const char base = 'A';
    CMsg res;
    memset(&res, 0, sizeof(CMsg));
    char dummy, inp, buff[20];      // no message is longer than 19
    res.type = CERROR;
    if (!fgets(buff, 20, f)) {
        res.type = HEOF;
        return res;
    }
    if (buff[strlen(buff) - 1] == '\n') {
        buff[strlen(buff) - 1] = '\0';
    }
    if (decode_fixed(buff, &res)) {
        return res;
    }
    if (sscanf(buff, "badmsg%c%c", &res.subject, &dummy) == 1) {
        res.type = OBADMSG;
    } else if (sscanf(buff, "disco%c%c", &res.subject, &dummy) == 1) {
        res.type = ODISCO;
    } else if (sscanf(buff, "driedout%c%c", &res.subject, &dummy) == 1) {
        res.type = RECOVERED;
    } else if (sscanf(buff, "hmove%c%c%c", &res.subject, &inp, &dummy) == 2) {
        res.type = MOVED_H;
        if (inp == '+') {
            res.value = 1;
        } else if (inp == '-') {
            res.value = -1;
        } else {
            res.type = CERROR;
        }
    } else if (sscanf(buff, "vmove%c%c", &res.subject, &dummy) == 1) {
        res.type = MOVED_V;
    } else if (sscanf(buff, "long%c%c%c", &res.subject, &res.object, &dummy)
            == 2) {
        res.type = TARGETED_L;
    } else if (sscanf(buff, "short%c%c%c", &res.subject, &res.object, &dummy)
            == 2) {
        res.type = TARGETED_S;
    } else if (sscanf(buff, "looted%c%c", &res.subject, &dummy) == 1) {
        res.type = LOOTED;
    } else if (sscanf(buff, "ordered%c%c%c", &res.subject, &res.order, &dummy)
            == 2) {
        res.type = ORDERED;
    } 
    res.subject -= base;
    if (res.object == '-') {
        res.missingObject = true;
    } else {
        res.object -= base;
    }
    return res;
}

// get a message and check that any players referenced exist 
// and orders are legal
CMsg get_message(FILE* f, int pCount)
{
    CMsg res = decode_message(f);
    switch (res.type) {
        case MOVED_H:
        case MOVED_V:
        case LOOTED:
        case TARGETED_S:
            if (res.subject >= pCount) {
                res.type = CERROR;
            }
            break;
        case TARGETED_L:
            if ((res.subject >= pCount)
                    || ((res.object >= pCount) && (!res.missingObject))) {
                res.type = CERROR;
            }
            break;
        case ORDERED:
            if ((res.subject >= pCount) || !is_order(res.order)) {
                res.type = CERROR;
            }
            break;
        default:
            ;
    }
    return res;
}

// Encode a message to clients into buffer
bool encode_message_fixed(char* buff, MType t)
{
    switch (t) {
        case HEOF:
        case CERROR:           // neither of these travel across network
            break;
        case GAME_OVER:
            strcpy(buff, "game_over");
            break;
        case ROUND:
            strcpy(buff, "round");
            break;
        case EXECUTE_PHASE:
            strcpy(buff, "execute");
            break;
        case TURN:
            strcpy(buff, "yourturn");
            break;
        case CHOOSE_H:
            strcpy(buff, "h?");
            break;
        case CHOOSE_LONG:
            strcpy(buff, "l?");
            break;
        case CHOOSE_SHORT:
            strcpy(buff, "s?");
            break;
        case ACK:
            sprintf(buff, "yes");
            break;
        case NACK:
            sprintf(buff, "no");
            break;
	default:
	    return false;
    }
    return true;
}

// Encode a message to clients into buffer
void encode_message(char* buff, CMsg m)
{
    if (encode_message_fixed(buff, m.type)) {
        return;
    }
    m.subject += 'A';
    buff[0] = 0;
    switch (m.type) {
        case OBADMSG:
            sprintf(buff, "badmsg%c", m.subject);
            break;
        case ODISCO:
            sprintf(buff, "disco%c", m.subject);
            break;
        case ORDERED:
            sprintf(buff, "ordered%c%c", m.subject, m.order);
            break;
        case MOVED_H:
            sprintf(buff, "hmove%c%c", m.subject, (m.value == 1) ? '+' : '-');
            break;
        case MOVED_V:
            sprintf(buff, "vmove%c", m.subject);
            break;
        case TARGETED_L:
            sprintf(buff, "long%c%c", m.subject, 
                    m.missingObject ? '-' : m.object + 'A');
            break;
        case TARGETED_S:
            sprintf(buff, "short%c%c", m.subject, 
                    m.missingObject ? '-' : m.object + 'A');
            break;
        case LOOTED:
            sprintf(buff, "looted%c", m.subject);
            break;
        case RECOVERED:
            sprintf(buff, "driedout%c", m.subject);
            break;
        default:
	    break;
    }
}

const char* mtype_to_str(MType m) {
    switch (m) {
        case ACK: return "ACK";
        case NACK: return "NACK";
        case HEOF: return "HEOF";
        case CERROR: return "CERROR";
        case OBADMSG: return "OBADMSG";
        case ODISCO: return "ODISCO";
        case GAME_OVER: return "GAME_OVER";
        case ROUND: return "ROUND";
        case EXECUTE_PHASE: return "EXECUTE_PHASE";
        case TURN: return "TURN";
        case ORDERED: return "ORDERED";
        case CHOOSE_H: return "CHOOSE_H";
        case CHOOSE_LONG: return "CHOOSE_LONG";
        case CHOOSE_SHORT: return "CHOOSE_SHORT";
        case MOVED_H: return "MOVED_H";
        case MOVED_V: return "MOVED_V";
        case TARGETED_L: return "TARGETED_L";
        case TARGETED_S: return "TARGETED_S";
        case LOOTED: return "LOOTED";
        case RECOVERED: return "RECOVERED";
        default:
            return "????";
    }
}
