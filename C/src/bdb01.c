#include "../include/easy.h"

int write_diary_entry(const char *diary_text)
{
	DBT key, data;
	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));
	uint64_t timestamp=(uint64_t)time(NULL);
	key.data=&timestamp; 
	key.size=sizeof(uint64_t);
	data.data=(void *)diary_text;
	data.size=strlen(diary_text)+1;
	return db->put(db, NULL, &key, &data, 0);
}

static void read_diary_json()
{
	DBC *d;
	DBT key, data;
	int ret, first=1;
	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));
	char buf[1024];
	if (db->cursor(db, NULL, &d, 0)!=0) {write(ns, "[]", 2); return;}
	write(ns, "[", 1);
	while ((ret=d->get(d, &key, &data, DB_NEXT))==0){
		uint64_t entry_time=*(uint64_t *)key.data;
		const char *text=(const char *)data.data;
		time_t raw_time=(time_t)entry_time;
		struct tm *t=localtime(&raw_time);
		if (!first) write(ns, ",", 1); 
		first=0;
		int len=snprintf(buf, sizeof(buf), "\"[%04d-%02d-%02d %02d:%02d:%02d] %s\"", t->tm_year+1900, t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, text);
		write(ns, buf, len);
	}
	write(ns, "]", 1);
	d->close(d);
}

int dbcmd(char *str, char *code)
{
	int ret=0;
	if (code[0]=='w' && strlen(str)>0) write_diary_entry(str);
	else if (code[0]=='r') read_diary_json();
	return ret;
}
