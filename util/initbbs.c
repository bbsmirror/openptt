/* $Id$ */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "config.h"
#include "pttstruct.h"
#include "perm.h"

static void initDir() {
    mkdir("adm", 0755);
    mkdir("boards", 0755);
    mkdir("etc", 0755);
    mkdir("man", 0755);
    mkdir("man/boards", 0755);
    mkdir("out", 0755);
    mkdir("tmp", 0755);
}

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
	strcpy(b.title, ".... �U�����F��  �m�����M�I,�D�H�i�ġn");
	b.brdattr = BRD_GROUPBOARD;
	b.level = PERM_SYSOP;
	b.uid = 1;
	b.gid = 0;
	newboard(fp, &b);
	
	strcpy(b.brdname, "junk");
	strcpy(b.title, "�o�q �����C���K���U��");
	b.brdattr = BRD_NOTRAN;
	b.level = PERM_SYSOP;
	b.uid = 2;
	b.gid = 1;
	newboard(fp, &b);
	
	strcpy(b.brdname, "Security");
	strcpy(b.title, "�o�q �������t�Φw��");
	b.brdattr = BRD_NOTRAN;
	b.level = PERM_SYSOP;
	b.uid = 3;
	b.gid = 1;
	newboard(fp, &b);
	
	strcpy(b.brdname, "2...........");
	strcpy(b.title, ".... �U�����s��     ���i  ����  ���I");
	b.brdattr = BRD_GROUPBOARD;
	b.level = 0;
	b.uid = 4;
	b.gid = 0;
	newboard(fp, &b);
	
	strcpy(b.brdname, "ALLPOST");
	strcpy(b.title, "�T�� ����O��LOCAL�s�峹");
	b.brdattr = BRD_POSTMASK | BRD_NOTRAN;
	b.level = PERM_SYSOP;
	b.uid = 5;
	b.gid = 4;
	newboard(fp, &b);
	
	strcpy(b.brdname, "deleted");
	strcpy(b.title, "�T�� ���귽�^����");
	b.brdattr = BRD_NOTRAN;
	b.level = PERM_BM;
	b.uid = 6;
	b.gid = 4;
	newboard(fp, &b);
	
	strcpy(b.brdname, "Note");
	strcpy(b.title, "�T�� ���ʺA�ݪO�κq����Z");
	b.brdattr = BRD_NOTRAN;
	b.level = 0;
	b.uid = 7;
	b.gid = 4;
	newboard(fp, &b);
	
	strcpy(b.brdname, "Record");
	strcpy(b.title, "�T�� ���ڭ̪����G");
	b.brdattr = BRD_NOTRAN | BRD_POSTMASK;
	b.level = 0;
	b.uid = 8;
	b.gid = 4;
	newboard(fp, &b);
	
	strcpy(b.brdname, "SYSOP");
	strcpy(b.title, "�T�� �������n!");
	b.brdattr = BRD_POSTMASK | BRD_NOTRAN | BRD_NOZAP;
	b.level = 0;
	b.uid = 9;
	b.gid = 4;
	newboard(fp, &b);
	
	strcpy(b.brdname, "WhoAmI");
	strcpy(b.title, "�T�� �������A�q�q�ڬO�֡I");
	b.brdattr = BRD_NOTRAN;
	b.level = 0;
	b.uid = 10;
	b.gid = 4;
	newboard(fp, &b);
	
	strcpy(b.brdname, "EditExp");
	strcpy(b.title, "�T�� ���d�����F��Z��");
	b.brdattr = BRD_NOTRAN;
	b.level = 0;
	b.uid = 11;
	b.gid = 4;
	newboard(fp, &b);
	
	fclose(fp);
    }
}

static void initMan() {
    FILE *fp;
    fileheader_t f;
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    
    memset(&f, 0, sizeof(f));
    f.savemode = 0;
    strcpy(f.owner, "SYSOP");
    sprintf(f.date, "%2d/%02d", tm->tm_mon + 1, tm->tm_mday);
    f.money = 0;
    f.filemode = 0;
    
    if((fp = fopen("man/boards/Note/.DIR", "w"))) {
	strcpy(f.filename, "SONGBOOK");
	strcpy(f.title, "�� �i�I �q �q ���j");
	fwrite(&f, sizeof(f), 1, fp);
	mkdir("man/boards/Note/SONGBOOK", 0755);
	
	strcpy(f.filename, "SONGO");
	strcpy(f.title, "�� <�I�q> �ʺA�ݪO");
	fwrite(&f, sizeof(f), 1, fp);
	mkdir("man/boards/Note/SONGO", 0755);
	
	strcpy(f.filename, "SYS");
	strcpy(f.title, "�� <�t��> �ʺA�ݪO");
	fwrite(&f, sizeof(f), 1, fp);
	mkdir("man/boards/Note/SYS", 0755);
	
	strcpy(f.filename, "AD");
	strcpy(f.title, "�� <�s�i> �ʺA�ݪO");
	fwrite(&f, sizeof(f), 1, fp);
	mkdir("man/boards/Note/AD", 0755);
	
	strcpy(f.filename, "NEWS");
	strcpy(f.title, "�� <�s�D> �ʺA�ݪO");
	fwrite(&f, sizeof(f), 1, fp);
	mkdir("man/boards/Note/NEWS", 0755);
	
	fclose(fp);
    }
    
}

static void initSymLink() {
    symlink(BBSHOME "/man/boards/Note/SONGBOOK", BBSHOME "/etc/SONGBOOK");
    symlink(BBSHOME "/man/boards/Note/SONGO", BBSHOME "/etc/SONGO");
    symlink(BBSHOME "/man/boards/EditExp", BBSHOME "/etc/editexp");
}

static void initHistory() {
    FILE *fp = fopen("etc/history.data", "w");
    
    if(fp) {
	fprintf(fp, "0 0 0 0");
	fclose(fp);
    }
}

int main() {
    if(chdir(BBSHOME)) {
	perror(BBSHOME);
	exit(1);
    }
    
    initDir();
    initHome();
    initPasswds();
    initBoards();
    initMan();
    initSymLink();
    initHistory();
    
    return 0;
}