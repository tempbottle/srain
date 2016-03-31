#ifndef __META_H
#define __META_H

#define META_NAME           "Srain"
#define META_VERSION        "0.01"
#define META_NAME_VERSION   "Srain 0.01"
#define META_DESC           "It does not look like a IRC client."
#define META_AUTHOR_NAME    "LastAvengers"
#define META_AUTHOR_MAIL    "lastavengers@outlook.com"
#define META_WEBSITE        "https://github.com/LastAvenger/srain"

#define META_CMD_HELP {\
    "Srain supports the following commands:\n"\
    "\n"\
    "/connect <server>: connect to a IRC server\n"\
    "\te.g: /connect irc.freenode.net\n"\
    "\tnb: you can connect to one server in the same time.\n"\
    "/login <nick>: login \n"\
    "\te.g: /login srainbot\n"\
    "/relaybot <nick>|<left delim>|<right delim>: regard <nick> as a relaybot\n"\
    "\te.g: /relaybot xmppbot|[|] \n"\
    "\tnb: delim is a string, not a char.\n"\
    "/ignore <nick>: ignore someone\n"\
    "\te.g: /ignore srainbot\n"\
    "\tnb: humans behind relaybot also can be ignore.\n"\
    "/join <channel>: join a channel\n"\
    "/part [channel]: leave a channel\n"\
    "\tnb: if [channel] no specified, leave current channel.\n"\
    "/quit: exit server\n"\
    "/me <msg>: send a action message\n"\
    "/msg <nick/channel> <msg>: send <msg> to <nick/channel>\n"\
    "/nick <nick>: change you nickname to <nick>\n"\
    "/whois <nick>: get <nick>'s WHOIS message\n"\
    "/invite <nick> <channel>: invite <nick> into <channel>\n"\
    "/kick <nick> [channel] [reason]: kick <nick> from <channel>\n"\
    "\tnb: if [channel] no specified, kick <nick> from current channel.\n"\
    "/mode <nick/channel> [flag]: get/set mode of <nick/channel>"\
}


/* META_SERVER is a sepecial name repersents
 * the IRC server you connecting to */
#define META_SERVER     "Server "
/* META_ABOUT is a sepecial name repersents
 * a About page */
// TODO: About is a legal nickname :(
#define META_ABOUT      "About "

#endif /* __META_H */