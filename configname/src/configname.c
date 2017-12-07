/*##################################################################################################################*/






/*###################################################################################################################*/


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
#include <time.h>
#include <sqlite3.h>

/*######################################################################################################################*/
	int checker(char input[],char check[])
		{
    		int i,result=1;
    			for(i=0;input[i]!='\0' && check[i]!='\0';i++)
    			{
        			if(input[i]!=check[i])
        				{
            				result=0;
            				break;
        				}
    			}
    		return result;
		}

	int status = 0;

	static int callback(void *data, int argc, char **argv, char **azColName)
		{
   			int i;
   			fprintf(stderr, "%s: ",(const char*)data);
   			char ti[100];
  			char *token;
  			char collegename[100];
      		char date[6][6];
   				for(i = 0; i<argc; i++)
   				{
      				printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   				}
			strcpy(ti,*argv);   
			printf("query:%s\n",ti);
        	mlx_proto_change_college_name(ti);
   			return 0;
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
				mlx_proto_change_rolling_text(1, " ");
				sleep(1);
				system("mlx_comm_test -l 3  red off ");
				sleep(2);
				system("mlx_comm_test -l 3  green off ");
				sleep(2);
				system("mlx_comm_test -l 3  green on ");
				sleep(2);
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
				//mlx_proto_change_rolling_text(1, " ");
				//sleep(1);		
				return 0;
		    }
		return 0;
	}
	

/*###############################################################################################################################*/

	int main()

		{
			char *p;
			FILE *in,*in1,*in2,*in3;
			extern FILE *popen();
			char buff[512]="\0";
			char buff1[512];
			char buff2[512];
			sqlite3 *db;
		   	char *zErrMsg = 0;
		   	int rc;
		   	char *sql;
		   	const char* data = "Callback function called";
			char str[20][100];
			int i = 0,j=2;
			int length_string;
			char file_str[8];
			char *auth;
			char curl_secret[38] = "curl -H \"Authorization: Bearer ";
			char curl_sec_final[280]="\0";
			char curl_sec_url[100] ="curl http://182.156.253.51:26/bucket/api/configfilecrc/14:3D:F2:BD:20:06/";
			char cur_sec_url_file[90]="curl http://182.156.253.51:26/bucket/api/configfile/14:3D:F2:BD:20:06/" ;
				
			buff[0] = '\0';
			buff1[0] = '\0';
			buff2[0] = '\0';
			curl_sec_final[0] = '\0';

				if(!(in1 = popen(curl_sec_url,"r")))
				{
					
				}	
					while(fgets(buff1, sizeof(buff1), in1)!=NULL)
					{
						pclose(in1);
					}
			
			printf("%s",buff1);
			strcat(curl_sec_final,cur_sec_url_file);
			strcat(curl_sec_final,buff1);
			strcat(curl_sec_final,"/");
			system("rm -r /www/usbshare/udisk/download/*");
			sleep(3);
			strcat(curl_sec_final," >/www/usbshare/udisk/download/config.zip");
			printf("Final:%s",curl_sec_final);

				if(!(in2 = popen(curl_sec_final,"r")))
					{
					
					}
					while(fgets(buff2, sizeof(buff2), in2)!=NULL)
					{
						pclose(in2);
					}

			sleep(5); 

			/*unzip config.zip get all fingerprint bin file and clean mnt location put all fingerprint bin file into mnt location*/

		 	chdir("/www/usbshare/udisk/download/");
		 	system("unzip -o config.zip");
			sleep(3);
		 	system("rm /mnt/*");
		 	sleep(2);
		 	system("cp -r fingerprint/* /mnt/");
			
				// senseconfig.zip contains senseconfigfile
			 	//system("zip -R /www/usbshare/udisk/download/senseconfig.zip \"*\"");

		 	/*Move to config folder and open config.db select specific EDC name and shows in LCD screen by callback function*/

			chdir("/www/usbshare/udisk/download/config");
		 	rc = sqlite3_open("config.db", &db);
		   
		  		 if( rc )
		  			{
		      			fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		      			return(0);
		   			} 
		   		else 
		   			{
		      			fprintf(stderr, "Opened database successfully\n");
		   			}

		   /* Create SQL statement */
		   sql = "SELECT edc_name from SYS";

		   /* Execute SQL statement */
		   rc = sqlite3_exec(db, sql, callback, (void*)data, &zErrMsg);
		   
		   		if( rc != SQLITE_OK ) 
		   			{
		      			fprintf(stderr, "SQL error: %s\n", zErrMsg);
		      			sqlite3_free(zErrMsg);
		   			} 
		   		else 
		   			{
		      			fprintf(stdout, "Operation done successfully\n");
		  			}
		  
			sleep(10);
			buff[0] = '\0';
			buff1[0] = '\0';
			buff2[0] = '\0';
			sqlite3_close(db);
		    return 0;
		  
		}

/*######################################################################################################################################*/
