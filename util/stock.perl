#!/usr/bin/perl
# ����]���ܡA�ݬ� bbspost �����|�O�_���T�C
# �p�G�o�X�� post �S����H���i�ӬO�� URL �䤣��A�h�T�w�@�U�ण��ݨ�
# ������H���� WWW �� URL �O�_���T�C
# �z�פW�A�ΩҦ� Eagle BBS �t�C�C
#                                       -- Beagle Apr 13 1997
#open(BBSPOST, "| /home/bbs/bin/post Record ����Ѯ�w�� [�氮�p��]");
open(BBSPOST, "| bin/webgrep >etc/stock.tmp");
#open(BBSPOST, ">/home/bbs/tmp/weather.tmp");
# ���
open(DATE, "date +'%a %b %d %T %Y' |");
$date = <DATE>;
chop $date;
close DATE;

# Header
# ���e
#open(WEATHER, "/usr/local/bin/lynx -dump http://www.dashin.com.tw/bulletin_board/today_stock_price.htm |"); while (<WEATHER>) {
open(WEATHER, "/usr/local/bin/lynx -dump http://quote.dashin.com.tw/today_stock_price.htm |"); while(<WEATHER>) {
  print BBSPOST if ($_ ne "\n");
}
close WEATHER;

# ñ�W��
print BBSPOST "\n--\n";
print BBSPOST "�ڬObeagle�Ҧ��i�R���p�氮...�����Ptt�A��\n";
print BBSPOST "--\n";
print BBSPOST "�� [Origin: ���G��p����] [From: [�Ų��P���]       ] ";

close BBSPOST;

