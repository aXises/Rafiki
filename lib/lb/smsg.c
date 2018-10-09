
#include <string.h>
#include "smsg.h"

// read message from specified stream
SMsg get_serv_message(FILE* f) {
    SMsg res;
    memset(&res, 0, sizeof(SMsg));
    char dummy;
    const char base = 'A';
    char inp;
    res.type = SERROR;
    char buffer[20];
    if (!fgets(buffer, 20, f)) {
        res.type = CEOF;
        return res;
    }
    size_t l = strlen(buffer);
    if (buffer[l - 1] == '\n') {
        buffer[l - 1] = '\0';
    }
    if (sscanf(buffer, "play%c%c", &res.order, &dummy) == 1) {
        res.type = PLAY;
    } else if (sscanf(buffer, "sideways%c%c", &inp, &dummy) == 1) {
        res.type = HMOVE;
        if ((inp != '+') && (inp != '-')) {
            res.type = SERROR;
        } else {
            res.value = (inp == '+') ? 1 : -1;
        }
    } else if (sscanf(buffer, "target_short%c%c", &res.object, &dummy) == 1) {
        res.type = STARGET;
        if (res.object == '-') {
            res.missingObject = true;
        } else {
            res.object -= base;
        }
    } else if (sscanf(buffer, "target_long%c%c", &res.object, &dummy) == 1) {
        res.type = LTARGET;
        if (res.object == '-') {
            res.missingObject = true;
        } else {
            res.object -= base;
        }
    }
    return res;
}

// return human readable name for message type enum value
const char* smtype_string(SMsgType t)
{
    switch (t) {
        case SERROR:
            return "SERROR";
        case CEOF:
            return "CEOF";
        case PLAY:
            return "PLAY";
        case HMOVE:
            return "HMOVE";
        case STARGET:
            return "STARGET";
        case LTARGET:
            return "LTARGET";
        default:
            return "?????";
    }
}
