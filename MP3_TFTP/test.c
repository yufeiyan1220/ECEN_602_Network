#include<stdio.h>
#include<string.h>
void TxtWrite_Cmode(int max)
{
	char a = 'a';
	FILE * fid = fopen("txt_out.txt","w");
	if(fid == NULL)
	{
		printf("failed in opening！\n");
		return;
	}
	for(int i = 0; i < max; i++ )
	{
		fprintf(fid, "%c", a);
	}
	fclose(fid);
}
void Txt_crlf()
{
	char a = 'a';
	FILE * fid = fopen("txt_out.txt","w");
	if(fid == NULL)
	{
		printf("failed in opening！\n");
		return;
	}
	for(int i = 0; i < 20; i ++ )
	{
		fprintf(fid, "\n");
		fprintf(fid, "%c", a);
		fprintf(fid, "\r");
	}
	fclose(fid);
}
void main(int argc, char** argv){
	char commandtext2048[] = "text2048";
	char commandtext2047[] = "text2047";
	char crlf[] = "crlf";
	if(argc != 2) printf("error happends");
	else { 
		if(!strcmp(argv[1], commandtext2048)) TxtWrite_Cmode(2048);
		else if(!strcmp(argv[1], commandtext2047)) TxtWrite_Cmode(2047);
		else if(!strcmp(argv[1], crlf)) Txt_crlf();
	}

}
