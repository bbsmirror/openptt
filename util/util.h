#ifndef INCLUDE_UTIL_H
#define INCLUDE_UTIL_H

int searchuser(char *userid);
int stampfile(char *fpath, fileheader_t *fh);
int append_record(char *fpath, fileheader_t *record, int size);
int get_record(char *fpath, void *rptr, int size, int id);
int substitute_record(char *fpath, void *rptr, int size, int id);
void resolve_boards();
int getbnum(char *bname);
void inbtotal(int bid, int add);
void *attach_shm(int shmkey, int shmsize);
void reload_pttcache();
void resolve_fcache();
void attach_uhash();
void stamplink(char *fpath, fileheader_t *fh);
void resolve_utmp();

#endif

