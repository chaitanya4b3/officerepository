#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "gen_types.h"
#include "zl-serial.h"
#include "libproto.h"
char *sys_call(char call[], int length)
{
	FILE *in;
	extern FILE *popen();
        
	char buff[1024];
	char *b;
	if(!(in = popen(call,"r")))
	{
			
	}	
	while(fgets(buff, sizeof(buff), in)!=NULL)
	{
		pclose(in);
	}
	
	b=buff;
	
	return b;

}

int checker(char input[],char check[])
{
    int i,result=1;
    for(i=0;input[i]!='\0' && check[i]!='\0';i++){
        if(input[i]!=check[i]){
            result=0;
            break;
        }
    }
    return result;
}

int ping_fun()
{
	FILE *fptr;
	char *sentence="connected";
	char *sentence1="not connected";
	if ( system("ping -c1 182.156.253.51") == 0)
    	{
		//create_log("Connected to the Cloud\n","file.log");
		mlx_proto_change_status_bar_icon(3, GUI_STATUS_ETH_CONN);
		sleep(1);
		mlx_proto_change_status_bar_icon(4, GUI_STATUS_CLOUD_CONN);
		sleep(1);
		system("mlx_comm_test -l 3  red off ");
		sleep(2);
		system("mlx_comm_test -l 3  green off ");
		sleep(2);
		system("mlx_comm_test -l 3  green on ");
		sleep(2);
		mlx_proto_change_rolling_text(1, " ");
		sleep(1); 	        	
		return 1;
                
                
                
    	}
    	else
    	{   
		//create_log("Connected failed with Cloud\n","file.log");
		mlx_proto_change_status_bar_icon(3, GUI_STATUS_ETH_DISCONN);
		sleep(1);
		mlx_proto_change_status_bar_icon(4, GUI_STATUS_CLOUD_DISCONN);
		sleep(1);
		mlx_proto_change_status_icon(GUI_STATUS_ICON_CLEAR);
		sleep(2);
		mlx_proto_change_rolling_text(1, " ");
		sleep(1);
		mlx_proto_change_rolling_text(1, "Network Not Connected...");
		sleep(1);
		system("mlx_comm_test -l 3  green off ");
		sleep(2);
		system("mlx_comm_test -l 3  red off ");
		sleep(2);
		system("mlx_comm_test -l 3  red on ");
		sleep(2);			
		return 0;
    	}
	return 0;
}

int main()
{
	char *p;
	char *x1,*x2,*x3;
	FILE *mac_id_get;
	char *pos;
	int bits=0;
	FILE *fp; //File pointer to open ipaddress
	char c[1000];
	char abc[100];	
	char mac_id[100];	
	fp = fopen("ipaddress.txt","r");
	char mac_url[30] = "uci get network.wan.macaddr";
	char str[20][100];
	int i = 0,j=2;
	int length_string;
	char file_str[8];
	char *auth;
	int connect_count = 0;
	char curl_sec_final[160]="\0";	
	char curl_sec_url[100] ="curl ";
	char crc_url[100] = "bucket/get_file_initial/";
	char crc_url1[100] = "bucket/get_file_crc/";
	fscanf(fp,"%[^\n]", c);
	
        fclose(fp);
	while(1)
	{
	bits=ping_fun();
	if(bits ==1)
	{	
	if(!(mac_id_get = popen(mac_url,"r")))
	{
			
	}
	while(fgets(mac_id, sizeof(mac_id), mac_id_get)!=NULL)
	{
		pclose(mac_id_get);
	}
	printf("mac id %s",mac_id);
	printf("IP address %s",c);
	if((pos=strchr(mac_id,'\n'))!=NULL)
	*pos=0;
		
	strcat(curl_sec_final,curl_sec_url);
	strcat(curl_sec_final,c);	
	strcat(curl_sec_final,crc_url);	

	mlx_proto_change_rolling_text(1, " ");
	sleep(2);
	mlx_proto_change_rolling_text(3, " ");
	sleep(2);
	mlx_proto_change_rolling_text(3, "Session Data Syncing...");
	sleep(2);
	
	
	strcat(curl_sec_final,mac_id);
	strcat(curl_sec_final,"/session/");
	printf("Data from the file:%s\n", curl_sec_final);
	x1 = sys_call(curl_sec_final,76);
	strcpy(abc,x1);
	printf("string :%s",x1);
	strncpy(curl_sec_final,"\0",sizeof(curl_sec_final));
	strcat(curl_sec_final,curl_sec_url);
	strcat(curl_sec_final,c);	
	strcat(curl_sec_final,crc_url1);
	strcat(curl_sec_final,mac_id);
	strcat(curl_sec_final,"/");
	strcat(curl_sec_final,abc);
	strcat(curl_sec_final,"/");
	strcat(curl_sec_final," >/www/usbshare/udisk/download/session.zip");
	printf("%s",curl_sec_final);
	x2 = sys_call(curl_sec_final,76);
	
	
	mlx_proto_change_rolling_text(3, " ");
	sleep(2);
	mlx_proto_change_rolling_text(3, "Session Data Synced with KenCloud");
	sleep(2);
	mlx_proto_change_status_icon(GUI_STATUS_ICON_RIGHT);
	sleep(3);
	mlx_proto_change_status_icon(GUI_STATUS_ICON_CLEAR);
	sleep(3);
	break;
	}
	else
	{
		connect_count++;	
			if(connect_count ==4)
			{
			mlx_proto_change_rolling_text(3, " ");	
			sleep(1);
			mlx_proto_change_rolling_text(3, "Check connectivity");	
	}

	}
}

}








