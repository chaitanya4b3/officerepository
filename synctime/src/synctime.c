#include<stdio.h>
#include<stdlib.h>
#include<string.h>
int main()
{
	char *p;
	FILE *in;
	char buff[20] = "\0";
	char curl_sec_url[70] = "curl http://182.156.253.51:26/sync_time/123/";
	char st1[40] = "mlx_comm_test -s ";
	if(!(in = popen(curl_sec_url,"r")))
	{
		
	}
	while(fgets(buff,sizeof(buff),in)!=NULL)
	{
		pclose(in);	
	}
strcat(st1,buff);
printf("String : %s",st1);
 system(st1);
//system("python json1.py");
}
