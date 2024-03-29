/* $Id$ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include "config.h"
#include "pttstruct.h"
#include "common.h"
#include "proto.h"

extern char *str_space;
extern int p_lines;             /* a Page of Screen line numbers: tlines-4 */
extern int b_lines;             /* Screen bottom line number: t_lines-1 */

word_t *toplev = NULL;
static word_t *current = NULL;
static char *msg_more = "\033[7m-- More --\033[m";

typedef char (*arrptr)[];
/* name complete for user ID */

static int UserMaxLen(char cwlist[][IDLEN + 1], int cwnum, int morenum,
		      int count) {
    int len, max = 0;
    
    while(count-- > 0 && morenum < cwnum) {
	len = strlen(cwlist[morenum++]);
	if (len > max)
	    max = len;
    }
    return max;
}

static int UserSubArray(char cwbuf[][IDLEN + 1], char cwlist[][IDLEN + 1],
			int cwnum, int key, int pos) {
    int key2, num = 0;
    int n, ch;

    key = chartoupper(key);
    if(key >= 'A' && key <= 'Z')
	key2 = key | 0x20;
    else
	key2 = key ;

    for(n = 0; n < cwnum; n++) {
	ch = cwlist[n][pos];
	if(ch == key || ch == key2)
	    strcpy(cwbuf[num++], cwlist[n]);
    }
    return num;
}

static void FreeNameList() {
    word_t *p, *temp;
    
    for(p = toplev; p; p = temp) {
	temp = p->next;
	free(p->word);
	free(p);
    }
}

void CreateNameList() {
    if(toplev)
	FreeNameList();
    toplev = current = NULL;
}

void AddNameList(char *name) {
    word_t *node;
    
    node = (word_t *)malloc(sizeof(word_t));
    node->next = NULL;
    node->word = (char *)malloc(strlen(name) + 1);
    strcpy(node->word, name);

    if(toplev)
	current = current->next = node;
    else
	current = toplev = node;
}

int RemoveNameList(char *name) {
    word_t *curr, *prev = NULL;

    for(curr = toplev; curr; curr = curr->next) {
	if(!strcmp(curr->word, name)) {
	    if(prev == NULL)
		toplev = curr->next;
	    else
		prev->next = curr->next;

	    if(curr == current)
		current = prev;
	    free(curr->word);
	    free(curr);
	    return 1;
	}
	prev = curr;
    }
    return 0;
}

int InNameList(char *name) {
    word_t *p;

    for(p = toplev; p; p = p->next)
	if(!strcmp(p->word, name))
	    return 1;
    return 0;
}

void ShowNameList(int row, int column, char *prompt) {
    word_t *p;
    
    move(row, column);
    clrtobot();
    outs(prompt);

    column = 80;
    for(p = toplev; p; p = p->next) {
	row = strlen(p->word) + 1;
	if(column + row > 76) {
	    column = row;
	    outc('\n');
	} else {
	    column += row;
	    outc(' ');
	}
	outs(p->word);
    }
}

void ToggleNameList(int *reciper, char *listfile, char *msg) {
    FILE *fp;
    char genbuf[200];

    if((fp = fopen(listfile, "r"))) {
	while(fgets(genbuf, STRLEN, fp)) {
	    strtok(genbuf, str_space);
	    if(!InNameList(genbuf)) {
		AddNameList(genbuf);
		(*reciper)++;
	    } else {
		RemoveNameList(genbuf);
		(*reciper)--;
	    }
	}
	fclose(fp);
	ShowNameList(3, 0, msg);
    }
}

static int NumInList(word_t *list) {
    register int i;

    for(i = 0; list; i++)
	list = list->next;
    return i;
}

int chkstr(char *otag, char *tag, char *name) {
    char ch, *oname = name;

    while(*tag) {
	ch = *name++;
	if(*tag != chartoupper(ch))
	    return 0;
	tag++;
    }
    if(*tag && *name == '\0')
	strcpy(otag, oname);
    return 1;
}

static word_t *GetSubList(char *tag, word_t *list) {
    word_t *wlist, *wcurr;
    char tagbuf[STRLEN];
    int n;

    wlist = wcurr = NULL;
    for(n = 0; tag[n]; n++)
	tagbuf[n] = chartoupper(tag[n]);
    tagbuf[n] = '\0';

    while(list) {
	if(chkstr(tag, tagbuf, list->word)) {
	    register word_t *node;

	    node = (word_t *)malloc(sizeof(word_t));
	    node->word = list->word;
	    node->next = NULL;
	    if(wlist)
		wcurr->next = node;
	    else
		wlist = node;
	    wcurr = node;
	}
	list = list->next;
    }
    return wlist;
}

static void ClearSubList(word_t *list) {
    struct word_t *tmp_list;

    while(list) {
	tmp_list = list->next;
	free(list);
	list = tmp_list;
    }
}

static int MaxLen(word_t *list, int count) {
    int len = strlen(list->word);
    int t;

    while(list && count) {
	if((t = strlen(list->word)) > len)
	    len = t;
	list = list->next;
	count--;
    }
    return len;
}

void namecomplete(char *prompt, char *data) {
    char *temp;
    word_t *cwlist, *morelist;
    int x, y, origx, origy;
    int ch;
    int count = 0;
    int clearbot = NA;
    
    if(toplev == NULL)
	AddNameList("");
    cwlist = GetSubList("", toplev);
    morelist = NULL;
    temp = data;
    
    outs(prompt);
    clrtoeol();
    getyx(&y, &x);
    getyx(&origy, &origx);
    standout();
    prints("%*s", IDLEN + 1, "");
    standend();
    move(y, x);
    refresh();
    
    while((ch = igetch()) != EOF) {
	if(ch == '\n' || ch == '\r') {
	    *temp = '\0';
	    outc('\n');
	    if(NumInList(cwlist) == 1)
		strcpy(data, cwlist->word);
	    ClearSubList(cwlist);
	    break;
	}
	if(ch == ' ') {
	    int col, len;
	    
	    if(NumInList(cwlist) == 1) {
		strcpy(data, cwlist->word);
		move(y, x);
		outs(data + count);
		count = strlen(data);
		temp = data + count;
		getyx(&y, &x);
		continue;
	    }
	    clearbot = YEA;
	    col = 0;
	    if(!morelist)
		morelist = cwlist;
	    len = MaxLen(morelist, p_lines);
	    move(2, 0);
	    clrtobot();
	    printdash("相關資訊一覽表");
	    while(len + col < 80) {
		int i;
		
		for(i = p_lines; (morelist) && (i > 0); i--) {
		    move(3 + (p_lines - i), col);
		    outs(morelist->word);
		    morelist = morelist->next;
		}
		col += len + 2;
		if(!morelist)
		    break;
		len = MaxLen(morelist, p_lines);
	    }
	    if(morelist) {
		move(b_lines, 0);
		outs(msg_more);
	    }
	    move(y, x);
	    continue;
	}
	if(ch == '\177' || ch == '\010') {
	    if(temp == data)
		continue;
	    temp--;
	    count--;
	    *temp = '\0';
	    ClearSubList(cwlist);
	    cwlist = GetSubList(data, toplev);
	    morelist = NULL;
	    x--;
	    move(y, x);
	    outc(' ');
	    move(y, x);
	    continue;
	}
	
	if(count < STRLEN && isprint(ch)) {
	    word_t *node;

	    *temp++ = ch;
	    count++;
	    *temp = '\0';
	    node = GetSubList(data, cwlist);
	    if(node == NULL) {
		temp--;
		*temp = '\0';
		count--;
		continue;
	    }
	    ClearSubList(cwlist);
	    cwlist = node;
	    morelist = NULL;
	    move(y, x);
	    outc(ch);
	    x++;
	}
    }
    if(ch == EOF)
	/* longjmp(byebye, -1); */
	raise(SIGHUP);	/* jochang: don't know if this is necessary... */
    outc('\n');
    refresh();
    if(clearbot) {
	move(2, 0);
	clrtobot();
    }
    if(*data) {
	move(origy, origx);
	outs(data);
	outc('\n');
    }
}

void usercomplete(char *prompt, char *data) {
    char *temp;
    char *cwbuf, *cwlist;
    int cwnum, x, y, origx, origy;
    int clearbot = NA, count = 0, morenum = 0;
    char ch;

    cwbuf = malloc(MAX_USERS * (IDLEN + 1));
    cwlist = u_namearray((arrptr)cwbuf, &cwnum, "");
    temp = data;
    
    outs(prompt);
    clrtoeol();
    getyx(&y, &x);
    getyx(&origy, &origx);
    standout();
    prints("%*s", IDLEN + 1, "");
    standend();
    move(y, x);
    while((ch = igetch()) != EOF) {
	if(ch == '\n' || ch == '\r') {
	    int i;
	    char *ptr;
	    
	    *temp = '\0';
	    outc('\n');
	    ptr = (char *)cwlist;
	    for(i = 0; i < cwnum; i++) {
		if(strncasecmp(data, ptr, IDLEN + 1) == 0)
		    strcpy(data, ptr);
		ptr += IDLEN + 1;
	    }
	    break;
	} else if(ch == ' ') {
	    int col, len;
	    
	    if(cwnum == 1) {
		strcpy(data, (char *)cwlist);
		move(y, x);
		outs(data + count);
		count = strlen(data);
		temp = data + count;
		getyx(&y, &x);
		continue;
	    }
	    clearbot = YEA;
	    col = 0;
	    len = UserMaxLen((arrptr)cwlist, cwnum, morenum, p_lines);
	    move(2, 0);
	    clrtobot();
	    printdash("使用者代號一覽表");
	    while(len + col < 79) {
		int i;
		
		for(i = 0; morenum < cwnum && i < p_lines; i++)	{
		    move(3 + i, col);
		    prints("%s ", cwlist + (IDLEN + 1) * morenum++);
		}
		col += len + 2;
		if(morenum >= cwnum)
		    break;
		len = UserMaxLen((arrptr)cwlist, cwnum, morenum, p_lines);
	    }
	    if(morenum < cwnum) {
		move(b_lines, 0);
		outs(msg_more);
	    } else
		morenum = 0;
	    move(y, x);
	    continue;
	} else if(ch == '\177' || ch == '\010') {
	    if(temp == data)
		continue;
	    temp--;
	    count--;
	    *temp = '\0';
	    cwlist = u_namearray((arrptr)cwbuf, &cwnum, data);
	    morenum = 0;
	    x--;
	    move(y, x);
	    outc(' ');
	    move(y, x);
	    continue;
	} else if(count < STRLEN && isprint(ch)) {
	    int n;
	    
	    *temp++ = ch;
	    *temp = '\0';
	    n = UserSubArray((arrptr)cwbuf, (arrptr)cwlist, cwnum, ch, count);
	    if(n == 0) {
		temp--;
		*temp = '\0';
		continue;
	    }
	    cwlist = cwbuf;
	    count++;
	    cwnum = n;
	    morenum = 0;
	    move(y, x);
	    outc(ch);
	    x++;
	}
    }
    free(cwbuf);
    if(ch == EOF)
	/* longjmp(byebye, -1); */
	raise(SIGHUP);	/* jochang: don't know if this is necessary */
    outc('\n');
    refresh();
    if(clearbot) {
	move(2, 0);
	clrtobot();
    }
    if(*data) {
	move(origy, origx);
	outs(data);
	outc('\n');
    }
}
