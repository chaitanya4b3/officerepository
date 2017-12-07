#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include "gen_types.h"
#include "zl-serial.h"
#include "libproto.h"
int main()
{

	char mac_url[30] = "uci get network.wan.macaddr";//
	FILE *mac_id_get,*get_instance;
	FILE *fptr;
	fptr = fopen("ipaddress.txt","w");
	char sys[6]="/sys/";
	char invalid[8] = "invalid";
	char json_string[20][100];	
	int i = 0;
	int error_count = 0; 
	int j=0;	
	char *pos;
	char instance_ip[20];
	char *token;
	char central_ken_instance[50]  = "http://182.156.253.51:28/instance/";
	char central_ken_instance_string[100] = "\0";
	char central_ken_instance_string_request[100]="\0";	
	char curl[5]="curl";	
	char mac_id[20];
	char instance_buffer[100];
	char status_string[7] = "\0";	
	while(1)
	{
	if(!(mac_id_get = popen(mac_url,"r")))
	{
			
	}
	while(fgets(mac_id, sizeof(mac_id), mac_id_get)!=NULL)
	{
		pclose(mac_id_get);
	}
	if((pos=strchr(mac_id,'\n'))!=NULL)
	*pos=0;
        strcpy(central_ken_instance_string,central_ken_instance);
	strcat(central_ken_instance_string,mac_id);
	//strcat(central_ken_instance_string,"/");
	strcat(central_ken_instance_string,sys);
	strcpy(central_ken_instance_string_request,"curl ");
	strcat(central_ken_instance_string_request,central_ken_instance_string);
	//printf("%s\n",central_ken_instance_string_request);
	
	
	if(!(get_instance = popen(central_ken_instance_string_request,"r")))
	{
	
	}
	while(fgets(instance_buffer,sizeof(instance_buffer),get_instance)!=NULL)
	{
		pclose(get_instance);	

	}	
	//printf("Request: %s\n",instance_buffer);
	token=strtok(instance_buffer,":");
	while(token!=NULL)
	{
		strcpy(json_string[i],token);
		printf("%s\n",token);
		token=strtok(NULL,",");	
		i++;
	
	}
		
	//printf("JSOn string:%s",json_string[1]);
	j =9;	
	for(i=0;i<4;i++)
	{
		status_string[i] = json_string[2][j];
		j++;	
	}
	//printf("Status %s:\n",status_string);
	if(!strcmp(json_string[1],"true"))
	{
		i=0;
		for(j=7;j<31;j++)
		{
			instance_ip[i] = json_string[2][j];
			i++;		
		}	
		//printf("Instance ip: %s\n",instance_ip);
		fprintf(fptr,"%s",instance_ip);
		mlx_proto_change_rolling_text(1, " ");
		sleep(1);
		mlx_proto_change_rolling_text(1, "Valid IntelliSYS");		
		sleep(10);
		fclose(fptr);
		
		sleep(10);
		break;

	}


	else
	{
			mlx_proto_change_rolling_text(1, " ");			
			sleep(1);			
			mlx_proto_change_rolling_text(1, "Invalid IntelliSYS,contact admin");		
			sleep(10);
			
	}
 }	
}


//http://192.168.2.26:28/instance/2C:AD:13:00:60:07/sys/

