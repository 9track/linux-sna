/* snatch_xwords.h: snatch XML word handlers.
 */

#ifndef _SNATCH_XWORDS_H
#define _SNATCH_XWORDS_H
struct wordmap {
        char *word;
        int val;
};

extern struct wordmap on_types[];
extern struct wordmap conversation_types[];
extern struct wordmap sync_levels[];

extern char *xword_pr_word(struct wordmap *wm, const int v);

extern int xword_on_off(xmlDocPtr doc, xmlNodePtr cur);
extern int xword_int(xmlDocPtr doc, xmlNodePtr cur);
extern int xword_strcpy(xmlDocPtr doc, xmlNodePtr cur, char *to);
extern int xword_strcpy_toupper(xmlDocPtr doc, xmlNodePtr cur, char *to);
extern int xword_min_max(xmlDocPtr doc, xmlNodePtr cur, xmlNsPtr ns, int *min, int *max);
extern int xword_conversation_type(xmlDocPtr doc, xmlNodePtr cur);
extern int xword_sync_level(xmlDocPtr doc, xmlNodePtr cur);
#endif	/* _SNATCH_XWORDS_H */
