#define char_lower(c)  ((c >= 'A' && c <= 'Z') ? c|32 : c)

/*void
str_lower(t, s)
  char *t, *s;*/
void str_lower(char *t, char *s)
{
  register char ch;
  do
  {
    ch = *s++;
    *t++ = char_lower(ch);
  } while (ch);
}

int
bad_subject(char *subject)
{
   char *badkey[] = {"�L�X","avcd","mp3",NULL};
   int i;
   for(i=0; badkey[i]; i++)
     if(strcasestr(subject, badkey[i])) return 1;
   return 0;
}           

