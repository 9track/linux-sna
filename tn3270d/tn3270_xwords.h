/* tn3270_xwords.h: tn3270 XML word handlers.
 */

#ifndef _TN3270_XWORDS_H
#define _TN3270_XWORDS_H
struct wordmap {
        char *word;
        int val;
};

extern struct wordmap on_types[];

extern char *xword_pr_word(struct wordmap *wm, const int v);

extern int xword_on_off(xmlDocPtr doc, xmlNodePtr cur);
extern int xword_int(xmlDocPtr doc, xmlNodePtr cur);
extern int xword_strcpy(xmlDocPtr doc, xmlNodePtr cur, char *to);
extern int xword_strcpy_toupper(xmlDocPtr doc, xmlNodePtr cur, char *to);
extern int xword_min_max(xmlDocPtr doc, xmlNodePtr cur, xmlNsPtr ns, int *min, int *max);
extern int xword_ip_allow(xmlDocPtr doc, xmlNodePtr cur, xmlNsPtr ns, server_info *server);
#endif	/* _SNA_XWORDS_H */
