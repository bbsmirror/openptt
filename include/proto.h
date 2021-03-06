/* $Id$ */
#ifndef INCLUDE_PROTO_H
#define INCLUDE_PROTO_H

/* admin */
int m_mod_board(char *bname);
int m_newbrd(int recover);
int scan_register_form(char *regfile, int automode, int neednum);
int m_user();
int search_user_bypwd();
int search_user_bybakpwd();
int m_board();
int m_register();
int cat_register();
unsigned int setperms(unsigned int pbits, char *pstring[]);

/* announce */
int a_menu(char *maintitle, char *path, int lastlevel);
void a_copyitem(char* fpath, char* title, char* owner, int mode);
int Announce();
void gem(char* maintitle, item_t* path, int update);

/* args */
void initsetproctitle(int argc, char **argv, char **envp);
void setproctitle(const char* format, ...);

/* bbs */
void make_blist();
int invalid_brdname(char *brd);
int del_range(int ent, fileheader_t *fhdr, char *direct);
int cmpfowner(fileheader_t *fhdr);
int b_note_edit_bname(int bid);
int Read();
void anticrosspost();
int Select();
void do_reply_title(int row, char *title);
int cmpfmode(fileheader_t *fhdr);
int cmpfilename(fileheader_t *fhdr);
int getindex(char *fpath, char *fname, int size);
void outgo_post(fileheader_t *fh, char *board);
int edit_title(int ent, fileheader_t *fhdr, char *direct);
void set_board();
int do_post();
void ReadSelect();
int save_violatelaw();
int board_select();
int board_etc();
int board_digest();

/* board */
int brc_unread(char *fname, int bnum, int *blist);
int brc_initial(char *boardname);
void brc_update();
int Ben_Perm(boardheader_t *bptr);
int New();
int Boards();
int root_board();

/* cache */
int getuser(char *userid);
void setuserid(int num, char *userid);
int searchuser(char *userid);
int getbnum(char *bname);
void reset_board(int bid);
void touch_boards();
void setapath(char *buf, char *boardname);
void setutmpmode(int mode);
void setadir(char *buf, char *path);
boardheader_t *getbcache(int bid);
int apply_boards(int (*func)(boardheader_t *));
int haspostperm(char *bname);
void inbtotal(int bid, int add);
void brc_addlist(char *fname);
void touchbtotal(int bid);
unsigned int safe_sleep(unsigned int seconds);
int apply_ulist(int (*fptr)(userinfo_t *));
userinfo_t *search_ulistn(int (*fptr)(int, userinfo_t *), int farg, int unum);
void purge_utmp(userinfo_t *uentp);
userinfo_t *search_ulist(int (*fptr)(int, userinfo_t *), int farg);
int count_multi();
void resolve_utmp();
void attach_uhash();
void getnewutmpent(userinfo_t *up);
void resolve_garbage();
void resolve_boards();
void resolve_fcache();
void sem_init(int semkey,int *semid);
void sem_lock(int op,int semid);
int count_ulist();
char *u_namearray(char buf[][IDLEN + 1], int *pnum, char *tag);
char *getuserid(int num);
int searchnewuser(int mode);
int count_logins(int (*fptr)(int, userinfo_t *), int farg, int show);
void remove_from_uhash(int n);
void add_to_uhash(int n, char *id);

/* cal */
int lockutmpmode(int unmode, int state);
int unlockutmpmode();
int p_touch_boards();
int x_file();

/* edit */
int vedit(char *fpath, int saveheader, int *islocal);
void write_header(FILE *fp);
void addsignature(FILE *fp, int ifuseanony);
void auto_backup();
void restore_backup();
char *ask_tmpbuf(int y);

/* friend */
void friend_edit(int type);
void friend_load();
int t_override();
int t_reject();
void friend_add(char *uident, int type);
void friend_delete(char *uident, int type);

/* io */
int getdata(int line, int col, char *prompt, char *buf, int len, int echo);
int igetch();
int getdata_str(int line, int col, char *prompt, char *buf, int len, int echo, char *defaultstr);
int getdata_buf(int line, int col, char *prompt, char *buf, int len, int echo);
int i_get_key();
void add_io(int fd, int timeout);
int igetkey();
void oflush();
int oldgetdata(int line, int col, char *prompt, char *buf, int len, int echo);
void output(char *s, int len);
void init_alarm();
int num_in_buf();
int ochar(int c);

/* kaede */
int Rename(char* src, char* dst);
int Link(char* src, char* dst);
char *Ptt_prints(char *str, int mode);
char *my_ctime(const time_t *t);

/* mail */
int mail_muser(userec_t muser, char *title, char *filename);
int mail_id(char* id, char *title, char *filename, char *owner);
int m_read();
int doforward(char *direct, fileheader_t *fh, int mode);
int mail_reply(int ent, fileheader_t *fhdr, char *direct);
int bsmtp(char *fpath, char *title, char *rcpt, int method);
void hold_mail(char *fpath, char *receiver);
int chkmail(int rechk);
void m_init();
int chkmailbox();
int mail_man();
int m_new();
int m_send();
int mail_list();
int setforward();
int m_internet();
int mail_mbox();
int built_mail_index();
int mail_all();
int invalidaddr(char *addr);
int do_send(char *userid, char *title);
void my_send(char *uident);

/* mbbsd */
void log_usies(char *mode, char *mesg);
void log_user(char *msg);
void abort_bbs(int sig);
void del_distinct(char *fname, char *line);
void add_distinct(char *fname, char *line);
void show_last_call_in(int save);
int dosearchuser(char *userid);
void u_exit(char *mode);

/* menu */
void showtitle(char *title, char *mid);
int egetch();
void movie(int i);
void domenu(int cmdmode, char *cmdtitle, int cmd, commands_t cmdtable[]);

/* more */
int more(char *fpath, int promptend);

/* name */
void usercomplete(char *prompt, char *data);
void namecomplete(char *prompt, char *data);
void AddNameList(char *name);
void CreateNameList();
int chkstr(char *otag, char *tag, char *name);
int InNameList(char *name);
void ShowNameList(int row, int column, char *prompt);
int RemoveNameList(char *name);
void ToggleNameList(int *reciper, char *listfile, char *msg);

/* osdep */
int cpuload(char *str);
double swapused(long *total, long *used);

/* read */
void i_read(int cmdmode, char *direct, void (*dotitle)(), void (*doentry)(), onekey_t *rcmdlist, int *num_record);
void fixkeep(char *s, int first);
keeploc_t *getkeep(char *s, int def_topline, int def_cursline);

/* record */
int substitute_record(char *fpath, void *rptr, int size, int id);
int get_record(char *fpath, void *rptr, int size, int id);
void prints(char *fmt, ...);
int append_record(char *fpath, fileheader_t *record, int size);
int stampfile(char *fpath, fileheader_t *fh);
void stampdir(char *fpath, fileheader_t *fh);
int get_num_records(char *fpath, int size);
int get_records(char *fpath, void *rptr, int size, int id, int number);
void stamplink(char *fpath, fileheader_t *fh);
int delete_record(char fpath[], int size, int id);
int delete_files(char* dirname, int (*filecheck)(), int record);
int delete_file(char *dirname, int size, int ent, int (*filecheck)());
int delete_range(char *fpath, int id1, int id2, char *title);
int apply_record(char *fpath, int (*fptr)(), int size);
int search_rec(char* dirname, int (*filecheck)());
int do_append(char *fpath, fileheader_t *record, int size);
int get_sum_records(char* fpath, int size);

/* register */
int getnewuserid();
int bad_user_id(char *userid);
void new_register();
int checkpasswd(char *passwd, char *test);
void check_register();
char *genpasswd(char *pw);

/* screen */
void move(int y, int x);
void outs(char *str);
void clrtoeol();
void clear();
void refresh();
void clrtobot();
void mprints(int y, int x, char *str);
void outmsg(char *msg);
void region_scroll_up(int top, int bottom);
void outc(unsigned char ch);
void redoscr();
void clrtoline(int line);
void standout();
void standend();
int edit_outs(char *text);
void outch(unsigned char c);
void rscroll();
void scroll();
void getyx(int *y, int *x);
void initscr();
void Jaky_outs(char *str, int line);

/* stuff */
void stand_title(char *title);
void pressanykey();
void trim(char *buf);
void bell();
void setbpath(char *buf, char *boardname);
int dashf(char *fname);
void sethomepath(char *buf, char *userid);
void sethomedir(char *buf, char *userid);
char *Cdate(time_t *clock);
void sethomefile(char *buf, char *userid, char *fname);
int log_file(char *filename,char *buf);
void str_lower(char *t, char *s);
int strstr_lower(char *str, char *tag);
int cursor_key(int row, int column);
int search_num(int ch, int max);
void setuserfile(char *buf, char *fname);
int is_BM(char *list);
long dasht(char *fname);
int dashd(char *fname);
int invalid_pname(char *str);
void setbdir(char *buf, char *boardname);
void setbfile(char *buf, char *boardname, char *fname);
int dashl(char *fname);
char *subject(char *title);
int not_alnum(char ch);
void setdirpath(char *buf, char *direct, char *fname);
int str_checksum(char *str);
void show_help(char *helptext[]);
int belong(char *filelist, char *key);
char *Cdatedate(time_t *clock);
int isprint2(char ch);
void sethomeman(char *buf, char *userid);
off_t dashs(char *fname);
void cursor_clear(int row, int column);
void cursor_show(int row, int column);
void printdash(char *mesg);
char *Cdatelite(time_t *clock);
int not_alpha(char ch);
int valid_ident(char *ident);
int userid_is_BM(char *userid, char *list);
int is_uBM(char *list, char *id);

/* syspost */
void post_newboard(char *bgroup, char *bname, char *bms);
void post_violatelaw(char *crime, char *police, char *reason, char *result);
void post_change_perm(int oldperm, int newperm, char *sysopid, char *userid);

/* talk */
int t_idle();
char *modestring(userinfo_t * uentp, int simple);
int is_friend(userinfo_t * ui);
int is_rejected(userinfo_t * ui);
int isvisible(userinfo_t * uentp, int isfri, int isrej);
int t_users();
int cmpuids(int uid, userinfo_t * urec);
int my_write(pid_t pid, char *hint, char *id, int flag);
void t_display_new();
void talkreply();
int t_monitor();
int t_pager();
int t_query();
int t_talk();
int t_display();
int my_query(char *uident);

/* term */
void init_tty();
int term_init();
void save_cursor();
void restore_cursor();
void do_move(int destcol, int destline);
void scroll_forward();
void change_scroll_range(int top, int bottom);

/* user */
void user_display(userec_t *u, int real);
void uinfo_query(userec_t *u, int real, int unum);
int showsignature(char *fname);
void mail_violatelaw(char* crime, char* police, char* reason, char* result);
void showplans(char *uid);
int u_info();
int u_loginview();
int u_ansi();
int u_editplan();
int u_editsig();
int u_switchproverb();
int u_editproverb();
int u_cloak();
int u_register();
int u_list();

/* vote */
int strip_ansi(char *buf, char *str, int mode);
void b_suckinfile(FILE *fp, char *fname);
int b_results();
int b_vote();
int b_vote_maintain();
int b_closepolls();

/* voteboard */
int do_voteboard();
void do_voteboardreply(fileheader_t *fhdr);

/* xyz */
int m_sysop();
int x_boardman();
int x_note();
int x_login();
int x_week();
int x_issue();
int x_today();
int x_yesterday();
int x_user100();
int x_birth();
int x_history();
int x_weather();
int x_stock();
int note();
int Goodbye();

/* toolkit */
unsigned StringHash(unsigned char *s);
int IsSNum(char *a);
int IsNum(char *a, int n);
int show_file(char *filename, int y, int lines, int mode);

/* passwd */
int passwd_mmap();
int passwd_update(int num, userec_t *buf);
int passwd_query(int num, userec_t *buf);
int passwd_apply(int (*fptr)(userec_t *));
void passwd_lock();
void passwd_unlock();

#endif
