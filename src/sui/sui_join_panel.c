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

/**
 * @file sui_join_panel.c
 * @brief Panel widget for join and search channel
 * @author Shengyu Zhang <i@silverrainz.me>
 * @version 0.06.2
 * @date 2016-09-16
 */

#include <gtk/gtk.h>

#include "sui/sui.h"
#include "srain.h"
#include "i18n.h"
#include "log.h"
#include "utils.h"

#include "sui_common.h"
#include "sui_event_hdr.h"
#include "sui_window.h"
#include "sui_buffer.h"
#include "sui_join_panel.h"

#define PAGE_JOIN_CHANNEL           "join_channel_page"
#define PAGE_SEARCH_CHANNEL         "search_channel_page"

#define MATCH_CHANNEL               0
#define MATCH_CHANNEL_WITH_REGEX    1
#define MATCH_TOPIC_WITH_REGEX      2

#define MATCH_LIST_STORE_COL_INDEX      0
#define MATCH_LIST_STORE_COL_COMMENT    1

#define CHANNEL_LIST_STORE_COL_CHANNEL  0
#define CHANNEL_LIST_STORE_COL_USERS    1
#define CHANNEL_LIST_STORE_COL_TOPIC    2

struct _SuiJoinPanel {
    GtkBox parent;

    bool is_adding; // Whether adding channels
    int match;

    GtkStack *stack;

    /* Join channel page */
    GtkEntry *chan_entry;
    GtkEntry *password_entry;

    /* Search channel page */
    GtkEntry *search_entry;
    GtkButton *refresh_button;
    /* Filter */
    GtkComboBox *match_combo_box;
    GtkSpinButton *min_users_spin_button;
    GtkSpinButton *max_users_spin_button;
    /* Channel list */
    GtkTreeView *chan_tree_view;
    GtkTreeViewColumn *chan_tree_view_column;
    GtkTreeViewColumn *users_tree_view_column;
    GtkTreeViewColumn *topic_tree_view_column;
    /* Status */
    GtkLabel *status_label;
    GtkSpinner *status_spinner;

    /* Buttons */
    GtkButton *cancel_button;
    GtkButton *join_button;

    /* Data model */
    GtkListStore *match_list_store;
};

struct _SuiJoinPanelClass {
    GtkBoxClass parent_class;
};

G_DEFINE_TYPE(SuiJoinPanel, sui_join_panel, GTK_TYPE_BOX);

static void match_combo_box_set_model(SuiJoinPanel *self);

static void cancel_button_on_click(gpointer user_data);
static void join_button_on_click(gpointer user_data);
static void match_combo_box_on_changed(GtkComboBox *combobox,
        gpointer user_data);
static void refresh_button_on_clicked(gpointer user_data);
static void chan_tree_view_on_row_activate(GtkTreeView *view,
        GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data);
gboolean chan_tree_visible_func(GtkTreeModel *model, GtkTreeIter *iter,
        gpointer user_data);
static void chan_tree_model_filter_refilter(gpointer user_data);
static void chan_tree_model_on_row_changed(GtkTreeModel *tree_model,
        GtkTreePath *path, GtkTreeIter *iter, gpointer user_data);

static void update_status(SuiJoinPanel *self);

/*****************************************************************************
 * GObject functions
 *****************************************************************************/

static void sui_join_panel_init(SuiJoinPanel *self){
    gtk_widget_init_template(GTK_WIDGET(self));

#if GTK_CHECK_VERSION(3, 18, 0)
    gtk_stack_set_interpolate_size(self->stack, TRUE);
#endif

    self->match = MATCH_CHANNEL;
    match_combo_box_set_model(self);

    /* Press enter to connect */
    g_signal_connect_swapped(self->chan_entry, "activate",
            G_CALLBACK(join_button_on_click), self);
    g_signal_connect_swapped(self->password_entry, "activate",
            G_CALLBACK(join_button_on_click), self);
    g_signal_connect_swapped(self->search_entry, "activate",
            G_CALLBACK(join_button_on_click), self);

    g_signal_connect_swapped(self->refresh_button, "clicked",
            G_CALLBACK(refresh_button_on_clicked), self);
    g_signal_connect(self->match_combo_box, "changed",
            G_CALLBACK(match_combo_box_on_changed), self);
    g_signal_connect(self->chan_tree_view, "row-activated",
            G_CALLBACK(chan_tree_view_on_row_activate), self);

    /* Filter condition changed */
    g_signal_connect_swapped(self->search_entry, "changed",
            G_CALLBACK(chan_tree_model_filter_refilter), self);
    g_signal_connect_swapped(self->match_combo_box, "changed",
            G_CALLBACK(chan_tree_model_filter_refilter), self);
    g_signal_connect_swapped(self->min_users_spin_button, "changed",
            G_CALLBACK(chan_tree_model_filter_refilter), self);
    g_signal_connect_swapped(self->max_users_spin_button, "changed",
            G_CALLBACK(chan_tree_model_filter_refilter), self);
    g_signal_connect_swapped(self->max_users_spin_button, "changed",
            G_CALLBACK(chan_tree_model_filter_refilter), self);

    g_signal_connect_swapped(self->cancel_button, "clicked",
            G_CALLBACK(cancel_button_on_click), self);
    g_signal_connect_swapped(self->join_button, "clicked",
            G_CALLBACK(join_button_on_click), self);
}

static void sui_join_panel_class_init(SuiJoinPanelClass *class){
    GtkWidgetClass *widget_class;

    widget_class = GTK_WIDGET_CLASS(class);
    gtk_widget_class_set_template_from_resource(widget_class,
            "/im/srain/Srain/join_panel.glade");

    gtk_widget_class_bind_template_child(widget_class, SuiJoinPanel, stack);

    gtk_widget_class_bind_template_child(widget_class, SuiJoinPanel, chan_entry);
    gtk_widget_class_bind_template_child(widget_class, SuiJoinPanel, password_entry );

    gtk_widget_class_bind_template_child(widget_class, SuiJoinPanel, search_entry);
    gtk_widget_class_bind_template_child(widget_class, SuiJoinPanel, refresh_button);
    gtk_widget_class_bind_template_child(widget_class, SuiJoinPanel, match_combo_box);
    gtk_widget_class_bind_template_child(widget_class, SuiJoinPanel, min_users_spin_button);
    gtk_widget_class_bind_template_child(widget_class, SuiJoinPanel, max_users_spin_button);

    gtk_widget_class_bind_template_child(widget_class, SuiJoinPanel, chan_tree_view);
    gtk_widget_class_bind_template_child(widget_class, SuiJoinPanel, chan_tree_view_column);
    gtk_widget_class_bind_template_child(widget_class, SuiJoinPanel, users_tree_view_column);
    gtk_widget_class_bind_template_child(widget_class, SuiJoinPanel, topic_tree_view_column);

    gtk_widget_class_bind_template_child(widget_class, SuiJoinPanel, status_label);
    gtk_widget_class_bind_template_child(widget_class, SuiJoinPanel, status_spinner);

    gtk_widget_class_bind_template_child(widget_class, SuiJoinPanel, cancel_button);
    gtk_widget_class_bind_template_child(widget_class, SuiJoinPanel, join_button);
}

/*****************************************************************************
 * Exported functions
 *****************************************************************************/

SuiJoinPanel* sui_join_panel_new(){
    return g_object_new(SUI_TYPE_JOIN_PANEL, NULL);
}

void sui_join_panel_clear(SuiJoinPanel *self){
    /* Clear join channel page input */
    gtk_entry_set_text(self->chan_entry, "");
    gtk_entry_set_text(self->password_entry, "");

    /* Clear search channel page input */
    gtk_entry_set_text(self->search_entry, "");
    gtk_spin_button_set_value(self->min_users_spin_button, -1);
    gtk_spin_button_set_value(self->max_users_spin_button, -1);
}

void sui_join_panel_set_model(SuiJoinPanel *self, GtkTreeModel *model){
    GtkTreeModelFilter *filter;

    filter = GTK_TREE_MODEL_FILTER(gtk_tree_model_filter_new(model, NULL));

    gtk_tree_model_filter_set_visible_func(filter,
            chan_tree_visible_func, self, NULL);
    gtk_tree_view_set_model(self->chan_tree_view, GTK_TREE_MODEL(filter));

    g_signal_connect(model, "row-inserted",
            G_CALLBACK(chan_tree_model_on_row_changed), self);
    g_signal_connect(model, "row-changed",
            G_CALLBACK(chan_tree_model_on_row_changed), self);
}

void sui_join_panel_set_is_adding(SuiJoinPanel *self, bool is_adding) {
    self->is_adding = is_adding;
    update_status(self);
}

bool sui_join_panel_get_is_adding(SuiJoinPanel *self){
    return self->is_adding;
}

/*****************************************************************************
 * Static functions
 *****************************************************************************/

static void match_combo_box_set_model(SuiJoinPanel *self){
    GtkListStore *store;
    GtkComboBox *combobox;
    GtkTreeIter iter;

    /* 2 columns: index, comment */
    self->match_list_store = gtk_list_store_new(2,
            G_TYPE_INT,
            G_TYPE_STRING);
    store = self->match_list_store;
    combobox = self->match_combo_box;

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter,
            MATCH_LIST_STORE_COL_INDEX, MATCH_CHANNEL,
            MATCH_LIST_STORE_COL_COMMENT, _("Match channel name"),
            -1);

    /* TODO
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter,
            MATCH_LIST_STORE_COL_INDEX, MATCH_CHANNEL_WITH_REGEX,
            MATCH_LIST_STORE_COL_COMMENT, _("Match channel name with regular expression"),
            -1);
    */

    gtk_combo_box_set_model(combobox, GTK_TREE_MODEL(store));
}

static void join_button_on_click(gpointer user_data){
    const char *page;
    const char *chan;
    const char *passwd;
    GVariantDict *params;
    g_autofree char *_chan = NULL;
    SrnRet ret;
    SuiJoinPanel *self;
    SuiBuffer *buf;

    self = user_data;
    buf = sui_common_get_cur_buffer(); // FIXME
    if (!SUI_IS_BUFFER(buf)){
       sui_message_box(_("Error"),
               _("Please connect to server before joining any channel"));
       return;
    }

    page = gtk_stack_get_visible_child_name(self->stack);
    if (g_strcmp0(page, PAGE_JOIN_CHANNEL) == 0){
        chan = gtk_entry_get_text(self->chan_entry);
        passwd = gtk_entry_get_text(self->password_entry);
    } else if (g_strcmp0(page, PAGE_SEARCH_CHANNEL) == 0){
        GtkTreeIter       iter;
        GtkTreeModel     *model;
        GtkTreeSelection *selection;

        model = gtk_tree_view_get_model(self->chan_tree_view);
        selection = gtk_tree_view_get_selection(self->chan_tree_view);
        if (gtk_tree_selection_get_selected(selection, &model, &iter)){
            /* If row in chan_tree_view has selected, use it as channel name */
            gtk_tree_model_get(model, &iter,
                    CHANNEL_LIST_STORE_COL_CHANNEL, &_chan,
                    -1);
            chan = _chan;
        } else {
            /* else, use value from search_entry */
            chan = gtk_entry_get_text(self->search_entry);
        }
        passwd = "";
    } else {
        ERR_FR("Unknown stack page: %s", page);
        return;
    }

    params = g_variant_dict_new(NULL);
    g_variant_dict_insert(params, "channel", SUI_EVENT_PARAM_STRING, chan);
    g_variant_dict_insert(params, "password", SUI_EVENT_PARAM_STRING, passwd);

    ret = sui_buffer_event_hdr(buf, SUI_EVENT_JOIN, params);
    g_variant_dict_unref(params);

    if (RET_IS_OK(ret)){
        gtk_button_clicked(self->cancel_button);
    } else {
        sui_message_box(_("Error"), RET_MSG(ret));
    }
}

static void cancel_button_on_click(gpointer user_data){
    SuiJoinPanel *self;

    self = user_data;

    sui_common_popdown_panel(GTK_WIDGET(self));
    sui_join_panel_clear(self);
}

static void match_combo_box_on_changed(GtkComboBox *combobox,
        gpointer user_data){
    int match;
    GtkTreeIter iter;
    SuiJoinPanel *self;

    self = user_data;

    if (!gtk_combo_box_get_active_iter(combobox, &iter)){
        ERR_FR("No acive item");
    }

    gtk_tree_model_get(GTK_TREE_MODEL(self->match_list_store), &iter,
            MATCH_LIST_STORE_COL_INDEX, &match,
            -1);
    self->match = match;

    DBG_FR("Selected index: %d", match);
}

static void refresh_button_on_clicked(gpointer user_data){
    SrnRet ret;
    SuiBuffer *buf;

    buf = sui_common_get_cur_buffer(); // FIXME:
    if (!SUI_IS_BUFFER(buf)){
       sui_message_box(_("Error"), _("Please connect to server before searching any channel"));
       return;
    }

    ret = sui_buffer_event_hdr(buf, SUI_EVENT_CHAN_LIST, NULL);
    if (!RET_IS_OK(ret)){
        sui_message_box(_("Error"), RET_MSG(ret));
    }
}


static void chan_tree_view_on_row_activate(GtkTreeView *view,
        GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data){
    char *chan = NULL;
    GtkTreeIter iter;
    GtkTreeModel *model;
    SuiJoinPanel *self;

    self = user_data;
    model = gtk_tree_view_get_model(view);
    if (!gtk_tree_model_get_iter(model, &iter, path)){
        ERR_FR("Failed to get GtkTreeIter from GtkTreePath");
        return;
    }

    gtk_tree_model_get(model, &iter,
            CHANNEL_LIST_STORE_COL_CHANNEL, &chan,
            -1);
    g_return_if_fail(chan);

    /* If a row is activated, set the value of chan_entry and switch to
     * PAGE_JOIN_CHANNEL. */
    gtk_entry_set_text(self->chan_entry, chan);
    gtk_stack_set_visible_child_name(self->stack, PAGE_JOIN_CHANNEL);

    g_free(chan);
}

static void chan_tree_model_filter_refilter(gpointer user_data){
    char *status;
    GtkTreeModel *model;
    GtkTreeModelFilter *filter;
    SuiJoinPanel *self;

    self = user_data;
    filter = GTK_TREE_MODEL_FILTER(gtk_tree_view_get_model(self->chan_tree_view));
    model = gtk_tree_model_filter_get_model(filter);

    gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(filter));

    /* Update status while all channels have loaded */
    status = g_strdup_printf(_("Showing %1$d of %2$d channnels"),
                gtk_tree_model_iter_n_children(GTK_TREE_MODEL(filter), NULL),
                gtk_tree_model_iter_n_children(model, NULL));
    gtk_label_set_text(self->status_label, status);
    g_free(status);
}

gboolean chan_tree_visible_func(GtkTreeModel *model, GtkTreeIter *iter,
        gpointer user_data){
    int users;
    int min_users;
    int max_users;
    char *chan = NULL;
    char *topic = NULL;
    const char *input;
    gboolean visable;
    SuiJoinPanel *self;

    visable = FALSE;
    self = user_data;
    gtk_tree_model_get(model, iter,
            CHANNEL_LIST_STORE_COL_CHANNEL, &chan,
            CHANNEL_LIST_STORE_COL_USERS, &users,
            CHANNEL_LIST_STORE_COL_TOPIC, &topic,
            -1);

    DBG_FR("cha: %s, users: %d, topic: %s", chan, users, topic);

    if (!chan) {
        goto FIN;
    }

    min_users = gtk_spin_button_get_value(self->min_users_spin_button);
    max_users = gtk_spin_button_get_value(self->max_users_spin_button);
    input = gtk_entry_get_text(self->search_entry);

    /* Filter users */
    if (min_users != - 1 && users < min_users){
        goto FIN;
    }
    if (max_users != - 1 && users > max_users){
        goto FIN;
    }

    switch (self->match){
        case MATCH_CHANNEL:
            if (str_is_empty(input) || g_strstr_len(chan, -1, input) != NULL){
                visable = TRUE;
            }
            break;
        case MATCH_CHANNEL_WITH_REGEX:
        default:
            ERR_FR("Unsupported match type");
    }

FIN:
    if (chan){
        g_free(chan);
    }
    if (topic){
        g_free(topic);
    }

    DBG_FR("visable: %d", visable);

    return visable;
}

static void chan_tree_model_on_row_changed(GtkTreeModel *tree_model,
        GtkTreePath *path, GtkTreeIter *iter, gpointer user_data){
    SuiJoinPanel *self;

    self = SUI_JOIN_PANEL(user_data);

    update_status(self);
}

static void update_status(SuiJoinPanel *self){
    int cur;
    int max;
    char *status;

    if (self->is_adding) {
        gtk_spinner_start(self->status_spinner);
    } else {
        gtk_spinner_stop(self->status_spinner);
    }

    if (gtk_tree_view_get_model(self->chan_tree_view)) {
        GtkTreeModel *model;
        GtkTreeModelFilter *filter;

        filter = GTK_TREE_MODEL_FILTER(
                gtk_tree_view_get_model(self->chan_tree_view));
        model = gtk_tree_model_filter_get_model(filter);
        cur = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(filter), NULL);
        max = gtk_tree_model_iter_n_children(model, NULL);
    } else {
        cur = 0;
        max = 0;
    }

    status  = g_strdup_printf(_("Showing %1$d of %2$d channnels"), cur, max);
    gtk_label_set_text(self->status_label, status);
    g_free(status);
}
