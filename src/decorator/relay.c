/* Copyright (C) 2016-2017 Shengyu Zhang <i@silverrainz.me>
 *
 * This file is part of Srain.
 *
 * Srain is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <glib.h>

#include "core/core.h"

#include "decorator.h"

#include "sirc/sirc.h"

#include "srain.h"
#include "log.h"
#include "i18n.h"
#include "utils.h"

static char* relay(SrnMessage *msg, int index, const char *frag);
static char* do_relay(GList *lst, SrnMessage *msg, const char *frag);

Decorator relay_decroator = {
    .name = "relay",
    .func = relay,
};
int relay_decroator_add_nick(SrnChat *chat, const char *nick){
    GList *lst;

    lst = chat->relaybot_list;

    while(lst){
        if (sirc_target_equal(lst->data, nick)){
            srn_chat_add_error_message_fmt(chat->srv->cur_chat, chat->user,
                    _("\"%1$s\" already exists in %2$s 's relaybot list"),
                    nick, chat->name);
            return SRN_ERR;
        }
        lst = g_list_next(lst);
    }

    chat->relaybot_list = g_list_append(chat->relaybot_list, g_strdup(nick));

    srn_chat_add_misc_message_fmt(chat->srv->cur_chat, chat->user,
            _("\"%1$s\" has added to %2$s 's relaybot list"), nick, chat->name);

    return SRN_OK;
}

int relay_decroator_rm_nick(SrnChat *chat, const char *nick){
    GList *lst;

    lst = chat->relaybot_list;

    while(lst){
        if (lst->data){
            if (sirc_target_equal(lst->data, nick)){
                g_free(lst->data);
                chat->relaybot_list = g_list_delete_link(chat->relaybot_list, lst);

                srn_chat_add_misc_message_fmt(chat->srv->cur_chat, chat->user,
                        _("\"%1$s\" is removed from %2$s 's relaybot list"),
                        nick, chat->name);

                return SRN_OK;
            }
        }
        lst = g_list_next(lst);
    }

    srn_chat_add_error_message_fmt(chat->srv->cur_chat, chat->user,
            _("\"%1$s\" not found in %2$s 's relaybot list"),
            nick, chat->name);

    return SRN_ERR;
}

void relay_decroator_free_list(SrnChat *chat){
    g_list_free_full(chat->relaybot_list, g_free);
    chat->relaybot_list = NULL;
}

static char* relay(SrnMessage *msg, int index, const char *frag){
    char *dcontent;

    if (index != 0) {
        DBG_FR("Only accept 0 while index = %d", index);
        return NULL;
    }

    dcontent = do_relay(msg->chat->relaybot_list, msg, frag);
    if (!dcontent) {
        dcontent = do_relay(msg->chat->srv->chat->relaybot_list, msg, frag);
    }

    return dcontent;
}

static char* do_relay(GList *lst, SrnMessage *msg, const char *frag){
    char *dnick;
    char *dcontent = NULL;
    GError *err;
    GRegex *regex;
    GMatchInfo *match_info;

    /* ref: https://github.com/tuna/scripts/blob/master/weechat_bot2human.py#L46 */
    err = NULL;
    regex = g_regex_new("(\x03[0-9,]+)?\\[(?<nick>[^:]+?)\\]\x0f? (?<text>.*)", 0, 0, &err);
    if (!regex){
        ERR_FR("g_regex_new() failed: %s", err->message);
        return NULL;
    }

    while (lst){
        if (sirc_target_equal(msg->user->srv_user->nick, lst->data)){
            DBG_FR("Brige bot '%s' found", (char *)lst->data);
            g_regex_match(regex, frag, 0, &match_info);

            if (g_match_info_matches(match_info)){
                char *tmp;
                char *escape_dnick;

                dnick = g_match_info_fetch_named(match_info, "nick");
                escape_dnick = g_markup_escape_text(dnick, -1);
                tmp = g_match_info_fetch_named(match_info, "text");

                str_assign(&msg->role, msg->dname);
                str_assign(&msg->dname, escape_dnick);
                dcontent = g_markup_escape_text(tmp, -1);

                LOG_FR("Relay message matched, nick: %s, content: %s", dnick, dcontent);

                g_free(tmp);
                g_free(dnick);
                g_free(escape_dnick);
                g_match_info_free(match_info);
                break;
            }
            g_match_info_free(match_info);
        }
        lst = g_list_next(lst);
    }

    g_regex_unref(regex);

    return dcontent;
}
