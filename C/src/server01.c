#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>

int ns;

static void catch_function(int signo)
{
	puts("\nInteractive attention signal caught.\n");
	exit(0);
}

void writeheader(int hd)
{
	char *h[2] = {"HTTP/1.1 200 OK\nContent-Type: ", "\n\n"};
	char *http[7] = {"text/css", "application/javascript", "text/plain", "text/html", "text/json", "application/wasm", "HTTP/1.1 204 No Content\n\n"};
	char buf[512];
	if (hd<6){
		sprintf(buf, "%s%s%s", h[0], http[hd], h[1]);
		write(ns, buf, strlen(buf));
	}
	else if (hd==6){ 
		write(ns, http[6], strlen(http[6]));
	}
}

void servefile(char *fstr)
{
	FILE *f;
	if ((f=fopen(fstr, "r"))==NULL) return;
	long fs;
	char *buffer, *fsuffix;
	size_t res;

	fseek (f, 0 , SEEK_END);
	fs=ftell(f);
	rewind(f);

	if ((buffer=(char*)malloc (sizeof(char)*fs))==NULL){fclose(f);	return;}
	res=fread(buffer, 1, fs, f);
	if (res!=fs){free(buffer); fclose(f);	return;}
	fclose(f);
	
	fsuffix=strrchr(fstr, '.');
	if (!strncmp(fsuffix, ".css", 3)) writeheader(0);
	else if (!strncmp(fsuffix, ".js", 3)) writeheader(1);
	else if (!strncmp(fsuffix, ".mp4", 3)) writeheader(2);
	else if (!strncmp(fsuffix, ".wasm", 4)) writeheader(5);
	else writeheader(3);
	write(ns, buffer, fs);
	free(buffer);
}

void writefile(char *fstr, char *txt)
{
	FILE *f;
	if ((f=fopen(fstr, "w"))==NULL) return;
	fwrite(txt, 1, strlen(txt), f);
	fclose(f);
	writeheader(6);
}

unsigned char urldecode(char *str, int lstr)
{
	int i=0, l=0;
	unsigned char ret=0;
	while (i<lstr)
	{
		l=lstr-i-1;
		unsigned char dig=0;
		switch (str[i]){
			case '0': dig=0<<4*l; break;
			case '1': dig=1<<4*l; break;
			case '2': dig=2<<4*l; break;
			case '3': dig=3<<4*l; break;
			case '4': dig=4<<4*l; break;
			case '5': dig=5<<4*l; break;
			case '6': dig=6<<4*l; break;
			case '7': dig=7<<4*l; break;
			case '8': dig=8<<4*l; break;
			case '9': dig=9<<4*l; break;
			case 'a': case 'A': dig=10<<4*l; break;
			case 'b': case 'B': dig=11<<4*l; break;
			case 'c': case 'C': dig=12<<4*l; break;
			case 'd': case 'D': dig=13<<4*l; break;
			case 'e': case 'E': dig=14<<4*l; break;
			case 'f': case 'F': dig=15<<4*l; break;
		}
		ret+=dig;
		i++;
	}
	return ret;
}

void decodefstr(char *fstr)
{
	if (strchr(fstr, '%')==NULL) servefile(fstr);
	else 
	{
		int i, j, l=strlen(fstr);
		char pathstr[l];
		for (i=j=0; i<l; ){
			if (*(fstr+i)=='%'){unsigned char x=urldecode(fstr+i+1, 2); pathstr[j]=x; i+=3; j+=1; continue;}
			else {pathstr[j]=*(fstr+i); i++; j++;}
		}
		pathstr[j]='\0';
		servefile(pathstr);
	}
}

int getuistr(char *req)
{
	int j=0;
	char *pfirst, *plast, *code;
	if ((pfirst=strchr(req, '\1'))){pfirst+=1; plast=strchr(pfirst, '\2'); code=plast+1;}
	else {if ((pfirst=strchr(req, ' '))!=NULL){pfirst++; plast=strchr(pfirst, ' ');	j=1;}	else return -1;}
	char str[plast-pfirst];
	strncpy(str, pfirst, plast-pfirst);
	str[plast-pfirst]='\0';
	if (j) decodefstr(str);
	else if (code[0]!='s'){
    writeheader(code[1]-'0');//converts single char to int (0=48, 1=49, 2=50 etc, therefore 2='2'(50)-'0'(48))
    char buffer[4096]; 
    FILE *fp=popen(str, "r");
    if (fp==NULL){perror("popen failed"); return -1;}
    size_t bytesRead;
    while ((bytesRead=fread(buffer, 1, sizeof(buffer), fp))>0){write(ns, buffer, bytesRead);}
    pclose(fp);
	}
	else {//code[0]='s'(ave)
		writefile(code+1, str);
	}
	return 1;
}

int runserver(unsigned port)
{
	int s, i=0;
	struct sockaddr_in sa, ca;
	socklen_t len=sizeof(sa);
	sa.sin_family=AF_INET;
	sa.sin_port=htons(port);
	sa.sin_addr.s_addr=htonl(INADDR_ANY);
	if ((s=socket(AF_INET, SOCK_STREAM, 0))<0){perror("can't create socket");	exit(EXIT_FAILURE);}
	if (bind(s, (struct sockaddr*)&sa, sizeof(sa))<0){perror("can't bind to address"); exit(EXIT_FAILURE);}
	if (listen(s, 6)<0){perror("can't listen");	exit(EXIT_FAILURE);}
	while (1)
	{
		if ((ns=accept(s, (struct sockaddr*)&ca, &len))<0){perror("error on accept");	exit(1);}
		char req[0xffff]={0};
		if ((i=read(ns, req, 0xffff))<0) continue;
		getuistr(req);
		close(ns);
	}
}		

int main(int argc, char *argv[])
{	
	signal(SIGCHLD, SIG_IGN); //child finished signal is not waited but ignored - else zombie processes pile needlessly up
	if (signal(SIGINT, catch_function)==SIG_ERR) perror("signal handler: ");
	if (signal(SIGTERM, catch_function)==SIG_ERR) perror("signal handler: ");
	runserver(8083);
	return 0;
}
