#!/usr/bin/perl
# $Id$
#
# ����]���ܡA�ݬ� bbspost �����|�O�_���T�C
# �p�G�o�X�� post �S����H���i�ӬO�� URL �䤣��A�h�T�w�@�U�ण��ݨ�
# ������H���� WWW �� URL �O�_���T�C
# �z�פW�A�ΩҦ� Eagle BBS �t�C�C
#                                       -- Beagle Apr 13 1997
open(BBSPOST, "| bin/webgrep>etc/weather.tmp");
# ���
open(DATE, "date +'%a %b %d %T %Y' |");
$date = <DATE>;
chop $date;
close DATE;

# Header
# ���e
open(WEATHER, "/usr/local/bin/lynx -dump http://www.cwb.gov.tw/Data/forecast/W03.txt |");
while (<WEATHER>) {
  print BBSPOST if ($_ ne "\n");
}
close WEATHER;

# ñ�W��
print BBSPOST "\n--\n";
print BBSPOST "�ڬObeagle�Ҧ��i�R���p�氮...�����Ptt�A��\n";
print BBSPOST "--\n";
print BBSPOST "�� [Origin: ���G��p����] [From: [�Ų��P���]       ] ";

close BBSPOST;

