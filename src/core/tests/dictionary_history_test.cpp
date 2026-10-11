// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/dictionary_history.hpp"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
using xiphos::core::DictionaryHistory;
using xiphos::core::DictionaryLocation;
void check(bool condition, const char *message) { if (!condition) throw std::runtime_error(message); }
// Ownership-compatible linked-list shim for the actual GTK-free adapter harness.
using gchar = char; using gpointer = void *; using gboolean = bool;
struct GList { void *data; GList *next=nullptr; GList *prev=nullptr; explicit GList(void *value):data(value) {} };
GList *g_list_last(GList *head) { if (head) while (head->next) head=head->next; return head; }
unsigned g_list_length(GList *head) { unsigned size=0; for (;head;head=head->next) ++size; return size; }
GList *g_list_append(GList *head, void *data) { auto *node=new GList(data); if (!head) return node; auto *last=g_list_last(head); last->next=node; node->prev=last; return head; }
GList *g_list_delete_link(GList *head,GList *node) { if (node->prev) node->prev->next=node->next; else head=node->next; if (node->next) node->next->prev=node->prev; delete node; return head; }
void g_list_free_full(GList *head, void (*release)(void *)) { while (head) { auto *next=head->next; release(head->data); delete head; head=next; } }
char *g_strdup(const char *value) { auto *copy=static_cast<char *>(std::malloc(std::strlen(value)+1)); std::strcpy(copy,value); return copy; }
void g_free(void *value) { std::free(value); }
void gtk_widget_set_sensitive(bool *widget,bool sensitive) { *widget=sensitive; }
#define TRUE true
#define FALSE false
#define g_new0(type,count) static_cast<type *>(std::calloc(count,sizeof(type)))
namespace frozen {
bool back=false, forward=false;
struct Widgets { bool *button_dict_back=&back, *button_dict_forward=&forward; } widgets;
DictionaryLocation displayed;
void main_dict_history_add(const gchar *,const gchar *);
void main_display_dictionary(const char *module,const char *key) { displayed={module,key}; main_dict_history_add(module,key); }
#include "legacy_dictionary_history.inc"
void clear() { g_list_free_full(dict_history_back,dict_history_entry_free); g_list_free_full(dict_history_forward,dict_history_entry_free); dict_history_back=nullptr; dict_history_forward=nullptr; back=false; forward=false; }
std::vector<DictionaryLocation> locations() { std::vector<DictionaryLocation> result; for (auto *p=dict_history_back;p;p=p->next) { auto *e=static_cast<DictHistoryEntry *>(p->data); result.push_back({e->mod_name,e->key}); } for (auto *p=g_list_last(dict_history_forward);p;p=p->prev) { auto *e=static_cast<DictHistoryEntry *>(p->data); result.push_back({e->mod_name,e->key}); } return result; }
}
namespace actual {
bool back=false, forward=false;
struct Widgets { bool *button_dict_back=&back, *button_dict_forward=&forward; } widgets;
DictionaryLocation displayed;
void main_dict_history_add(const gchar *,const gchar *);
void main_display_dictionary(const char *module,const char *key) { displayed={module,key}; main_dict_history_add(module,key); }
#include "legacy_dictionary_history_actual.inc"
void clear() { g_list_free_full(dict_history_back,dict_history_entry_free); g_list_free_full(dict_history_forward,dict_history_entry_free); dict_history_back=nullptr; dict_history_forward=nullptr; back=false; forward=false; }
std::vector<DictionaryLocation> locations() { std::vector<DictionaryLocation> result; for (auto *p=dict_history_back;p;p=p->next) { auto *e=static_cast<DictHistoryEntry *>(p->data); result.push_back({e->mod_name,e->key}); } for (auto *p=g_list_last(dict_history_forward);p;p=p->prev) { auto *e=static_cast<DictHistoryEntry *>(p->data); result.push_back({e->mod_name,e->key}); } return result; }
}
#undef g_new0
bool equal(const std::vector<DictionaryLocation> &a,const std::vector<DictionaryLocation> &b) { if (a.size()!=b.size()) return false; for (std::size_t i=0;i<a.size();++i) if (a[i].module!=b[i].module || a[i].key!=b[i].key) return false; return true; }
int main() {
  try {
    DictionaryHistory history; unsigned cases=0;
    auto compare=[&] {
      check(equal(history.entries(),frozen::locations()) && equal(history.entries(),actual::locations()),"shared and actual dictionary entries equal frozen legacy");
      check(history.can_back()==frozen::back && history.can_forward()==frozen::forward && actual::back==frozen::back && actual::forward==frozen::forward,"dictionary history button parity");
      check(history.entries().empty() || history.cursor()+1==g_list_length(frozen::dict_history_back),"dictionary history cursor parity"); ++cases;
    };
    auto visit=[&](const DictionaryLocation &location) { history.visit(location); frozen::main_dict_history_add(location.module.c_str(),location.key.c_str()); actual::main_dict_history_add(location.module.c_str(),location.key.c_str()); compare(); };
    auto move=[&](int direction) { const bool allowed=direction==-1?history.can_back():history.can_forward(); if (allowed) history.move(direction); else { bool rejected=false; try { history.move(direction); } catch (const std::invalid_argument &) { rejected=true; } check(rejected,"shared boundary rejects without mutation"); } if (direction==-1) { frozen::main_dict_history_back(); actual::main_dict_history_back(); } else { frozen::main_dict_history_forward(); actual::main_dict_history_forward(); } compare(); if (allowed) { const auto &current=history.entries()[history.cursor()]; check(current.module==frozen::displayed.module && current.key==frozen::displayed.key && current.module==actual::displayed.module && current.key==actual::displayed.key,"history navigation displays expected target without recording"); } };
    visit({"A","alpha"}); visit({"B","alpha"}); visit({"B","café"}); move(-1); visit({"B","alpha"});
    check(history.can_forward(),"duplicate current preserves forward history");
    visit({"A","alpha"}); check(!history.can_forward(),"changed module/key clears forward history");
    for (unsigned i=0;i<60;++i) visit({"A",std::to_string(i)});
    check(history.entries().size()==50 && history.entries().front().key=="10","dictionary oldest eviction at fifty");
    for (unsigned i=0;i<60;++i) move(-1);
    for (unsigned i=0;i<60;++i) move(1);
    for (unsigned seed=1;seed<=100;++seed) {
      history.clear(); frozen::clear(); actual::clear(); compare(); unsigned random=seed;
      for (unsigned step=0;step<100;++step) {
        random=random*1664525u+1013904223u;
        const auto action=random%5;
        if (action==0) move(-1); else if (action==1) move(1);
        else visit({random%2?"A":"B",random%3==0?"café":std::to_string(random%7)});
      }
    }
    history.clear(); frozen::clear(); actual::clear(); compare();
    bool invalid=false; try { history.move(0); } catch (const std::invalid_argument &) { invalid=true; }
    check(invalid && history.entries().empty(),"invalid history direction retains state");
    std::cout << cases << " frozen/actual/shared dictionary history transitions passed\n";
  } catch (const std::exception &error) { frozen::clear(); actual::clear(); std::cerr<<error.what()<<'\n'; return 1; }
}
