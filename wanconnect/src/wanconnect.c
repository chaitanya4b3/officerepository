/*########################################################





##########################################################*/

#include<stdio.h>
#include<time.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include "gen_types.h"
#include "zl-serial.h"
#include "libproto.h"

/*########################################################

##########################################################*/
int ping_fun()
{
	if(system("ping -c1 182.156.253.51") == 0)
	{
		mlx_proto_change_status_bar_icon(3, GUI_STATUS_ETH_CONN);
		sleep(1);
		mlx_proto_change_status_bar_icon(4, GUI_STATUS_CLOUD_CONN);
		sleep(1);
		mlx_proto_change_rolling_text(1, " ");
		sleep(1);
		mlx_proto_change_rolling_text(1, "Network Connected");
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
}


int main()
	{
		FILE *fp;
		time_t current_time;
		char* c_time_string;
		current_time = time(NULL);
		c_time_string = ctime(&current_time);
		int status = 0;
		mlx_proto_change_rolling_text(1, " ");
		mlx_proto_change_status_bar_icon(3, GUI_STATUS_ETH_DISCONN);
		sleep(1);
		mlx_proto_change_status_bar_icon(4, GUI_STATUS_CLOUD_DISCONN);
		sleep(1);
		mlx_proto_change_status_bar_icon(5,GUI_STATUS_GPS_DISABLE);
		sleep(1);
		
		
				while(1)
				{
					status = ping_fun();
						if(status == 1)
							{
								system("instancemd");
								sleep(3);
								system("configname");
								
									while(1)
										{
											status = ping_fun();
												if(status == 1)
													{
														
														system("synctime");
														sleep(2);
														mlx_proto_change_rolling_text(1, "Time Synced with KenCloud");
														sleep(1);
														mlx_proto_change_status_icon(GUI_STATUS_ICON_RIGHT);
														sleep(3);
														mlx_proto_change_status_icon(GUI_STATUS_ICON_CLEAR);
														sleep(3);
														mlx_proto_change_rolling_text(1, " ");
														sleep(1);
															mlx_proto_change_rolling_text(1, "Geo Location Authenticated");
															sleep(3); 
															mlx_proto_change_status_bar_icon(5,GUI_STATUS_GPS_ENABLE);
															sleep(1); 
															mlx_proto_change_status_icon(GUI_STATUS_ICON_RIGHT);
															sleep(3);
															mlx_proto_change_status_icon(GUI_STATUS_ICON_CLEAR);
															sleep(3);
															mlx_proto_change_rolling_text(1," ");
															sleep(1);
																	
													break;				
							
													}	
												else
													{
														
													}	
															
									}
								break;	
							}		
							else
								{
									
									
									
								}
				}
	}

/*########################################################

##########################################################*/


