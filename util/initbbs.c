#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "config.h"
#include "struct.h"
#include "perm.h"

static void initHome() {
    int i;
    char buf[256];
    
    mkdir("home", 0755);
    strcpy(buf, "home/?");
    for(i = 0; i < 26; i++) {
	buf[5] = 'A' + i;
	mkdir(buf, 0755);
	buf[5] = 'a' + i;
	mkdir(buf, 0755);
    }
}

static void initPasswds() {
    int i;
    userec_t u;
    FILE *fp = fopen(".PASSWDS", "w");
    
    memset(&u, 0, sizeof(u));
    if(fp) {
	for(i = 0; i < MAX_USERS; i++)
	    fwrite(&u, sizeof(u), 1, fp);
	fclose(fp);
    }
}

static void newboard(FILE *fp, boardheader_t *b) {
    char buf[256];
    
    fwrite(b, sizeof(boardheader_t), 1, fp);
    sprintf(buf, "boards/%s", b->brdname);
    mkdir(buf, 0755);
    sprintf(buf, "man/boards/%s", b->brdname);
    mkdir(buf, 0755);
}

static void initBoards() {
    FILE *fp = fopen(".BOARDS", "w");
    boardheader_t b;
    
    if(fp) {
	memset(&b, 0, sizeof(b));
	
	strcpy(b.brdname, "1...........");
	strcpy(b.title, ".... Σ中央政府  《高壓危險,非人可敵》");
	b.brdattr = BRD_GROUPBOARD;
	b.level = PERM_SYSOP;
	b.uid = 1;
	b.gid = 0;
	newboard(fp, &b);
	
	strcpy(b.brdname, "junk");
	strcpy(b.title, "發電 ◎雜七雜八的垃圾");
	b.brdattr = BRD_NOTRAN;
	b.level = PERM_SYSOP;
	b.uid = 2;
	b.gid = 1;
	newboard(fp, &b);
	
	strcpy(b.brdname, "Security");
	strcpy(b.title, "發電 ◎站內系統安全");
	b.brdattr = BRD_NOTRAN;
	b.level = PERM_SYSOP;
	b.uid = 3;
	b.gid = 1;
	newboard(fp, &b);
	
	strcpy(b.brdname, "2...........");
	strcpy(b.title, ".... Σ市民廣場     報告  站長  ㄜ！");
	b.brdattr = BRD_GROUPBOARD;
	b.level = 0;
	b.uid = 4;
	b.gid = 0;
	newboard(fp, &b);
	
	strcpy(b.brdname, "ALLPOST");
	strcpy(b.title, "嘰哩 ◎跨板式LOCAL新文章");
	b.brdattr = BRD_POSTMASK | BRD_NOTRAN;
	b.level = PERM_SYSOP;
	b.uid = 5;
	b.gid = 4;
	newboard(fp, &b);
	
	strcpy(b.brdname, "deleted");
	strcpy(b.title, "嘰哩 ◎資源回收筒");
	b.brdattr = BRD_NOTRAN;
	b.level = PERM_BM;
	b.uid = 6;
	b.gid = 4;
	newboard(fp, &b);
	
	strcpy(b.brdname, "Note");
	strcpy(b.title, "嘰哩 ◎動態看板及歌曲投稿");
	b.brdattr = BRD_NOTRAN;
	b.level = 0;
	b.uid = 7;
	b.gid = 4;
	newboard(fp, &b);
	
	strcpy(b.brdname, "Record");
	strcpy(b.title, "嘰哩 ◎我們的成果");
	b.brdattr = BRD_NOTRAN | BRD_POSTMASK;
	b.level = PERM_SYSOP;
	b.uid = 8;
	b.gid = 4;
	newboard(fp, &b);
	
	strcpy(b.brdname, "SYSOP");
	strcpy(b.title, "嘰哩 ◎站長好!");
	b.brdattr = BRD_POSTMASK | BRD_NOTRAN | BRD_NOZAP;
	b.level = 0;
	b.uid = 9;
	b.gid = 4;
	newboard(fp, &b);
	
	strcpy(b.brdname, "WhoAmI");
	strcpy(b.title, "嘰哩 ◎呵呵，猜猜我是誰！");
	b.brdattr = BRD_NOTRAN;
	b.level = 0;
	b.uid = 10;
	b.gid = 4;
	newboard(fp, &b);
	
	fclose(fp);
    }
}

int main() {
    if(chdir(BBSHOME)) {
	perror(BBSHOME);
	exit(1);
    }
    
    mkdir("adm", 0755);
    mkdir("boards", 0755);
    mkdir("etc", 0755);
    mkdir("man", 0755);
    mkdir("man/boards", 0755);
    mkdir("out", 0755);
    mkdir("tmp", 0755);
    
    initHome();
    initPasswds();
    initBoards();
    
    return 0;
}
