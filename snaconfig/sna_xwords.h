/* sna_xwords.h: SNA XML word handlers.
 */

#ifndef _SNA_XWORDS_H
#define _SNA_XWORDS_H
struct wordmap {
        char *word;
        int val;
};

extern struct wordmap on_types[];
extern struct wordmap security_types[];
extern struct wordmap lu_types[];
extern struct wordmap tx_types[];
extern struct wordmap node_types[];
extern struct wordmap role_types[];
extern struct wordmap direction_types[];

extern char *xword_pr_word(struct wordmap *wm, const int v);

extern int xword_direction_types(xmlDocPtr doc, xmlNodePtr cur);
extern int xword_role_types(xmlDocPtr doc, xmlNodePtr cur);
extern int xword_node_types(xmlDocPtr doc, xmlNodePtr cur);
extern int xword_tx_types(xmlDocPtr doc, xmlNodePtr cur);
extern int xword_lu_types(xmlDocPtr doc, xmlNodePtr cur);
extern int xword_security(xmlDocPtr doc, xmlNodePtr cur);
extern int xword_on_off(xmlDocPtr doc, xmlNodePtr cur);
extern int xword_int(xmlDocPtr doc, xmlNodePtr cur);
extern int xword_strcpy(xmlDocPtr doc, xmlNodePtr cur, char *to);
extern int xword_strcpy_toupper(xmlDocPtr doc, xmlNodePtr cur, char *to);
extern unsigned char xword_lsap(xmlDocPtr doc, xmlNodePtr cur);
extern int xword_min_max(xmlDocPtr doc, xmlNodePtr cur, xmlNsPtr ns, int *min, int *max);
extern int xword_cost(xmlDocPtr doc, xmlNodePtr cur, xmlNsPtr ns, 
	int *min_connect, int *max_connect, int *min_byte, int *max_byte);
extern int xword_netid(xmlDocPtr doc, xmlNodePtr cur, sna_netid *netid);
extern sna_nodeid xword_nodeid(xmlDocPtr doc, xmlNodePtr cur);
extern int xword_mac_address(xmlDocPtr doc, xmlNodePtr cur, char *mac);
#endif	/* _SNA_XWORDS_H */
