#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
 #include <errno.h>
    #include <string.h>
    #include <strings.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <stdarg.h>
    #include <time.h>
    #include <signal.h>
    #include <fcntl.h>
    #include <semaphore.h>
    #include <linux/types.h>
    #include <sys/time.h>
    #include <sys/times.h>
    #include <sys/uio.h>
    #include <sys/un.h>
    #include <sys/ioctl.h>
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <sys/file.h>
    #include <sys/types.h>
    #include <sys/param.h>
#include <sys/socket.h>
    #include <sys/wait.h>
    #include <sys/utsname.h>
    #include <sys/epoll.h>
    #include <sys/ipc.h>
    #include <netinet/in.h>
	#include <netinet/tcp.h>
	#include <net/if.h>
    #include <netdb.h>
    #include <pthread.h>
    #include <unistd.h>
    #include <ctype.h>
    #include <sched.h>
	#include <termios.h>
	#include <arpa/inet.h>
    #include <linux/soundcard.h>
#include "gen_types.h"
#include "zl-serial.h"
#include "libproto.h"

#define MLX_INTELLISENSE	0
#define MLX_INTELLISYS		1

#define NET_PORT 8888
#define UDP_PORT 8080
#define MAXLINENO 3
#define RECVBUFSIZE 1024
#define READBUFSIZE 128
#define HEADER 0x80
#define FOOTER 0x5A
#define NEWDEV_TAG "NEW DEV"
#define GETXML_TAG "Get XML"
#define PC 0

#define MLX_PROTO_SERIAL		"/dev/ttyUSB%d,115200,8,N,1"

#if 1
#define PROTO_BUFF_LEN				2048
#define PROTO_DATA_LEN				256

#define PROTO_SOH_FF    			0xFF
#define PROTO_SOH_55    			0x55
#define PROTO_SOH_AA    			0xAA

#define MSB_LSB_SWAP16(A)     ((((u_int16_t)(A) & 0xff00) >> 8) | \
                    (((u_int16_t)(A) & 0x00ff) << 8))

typedef struct _Serialbuff Serailbuff;
struct _Serialbuff {
	int fd;
	unsigned char data[512];
	size_t size;
};

typedef enum
{
    BAT_STATUS_CHARGING=128,                    //0x80
    BAT_STATUS_CHARGING_75=136,                 //0x88
    BAT_STATUS_CHARGING_50=132,                 //0x84
    BAT_STATUS_CHARGING_25=130,                 //0x82
    BAT_STATUS_CHARGING_0=131,                  //0x81
    BAT_STATUS_PERCENT_100=15,                  //0x0f
    BAT_STATUS_PERCENT_75=7,                    //0x07
    BAT_STATUS_PERCENT_50=3,                    //0x03
    BAT_STATUS_PERCENT_25=1,                    //0x01
    BAT_STATUS_PERCENT_0=0,                     //0x00
} BATTERY_STAT;

typedef enum
{
    PROTO_WAIT_FF = 0,
    PROTO_WAIT_55 = 1,
    PROTO_WAIT_AA = 2,
    PROTO_WAIT_LEN = 3,
    PROTO_WAIT_TYPE = 4,
    PROTO_WAIT_DATA = 5,
    PROTO_WAIT_CRC = 6,
}PROTO_STAT;

typedef struct _PROTO_BUFF
{
	PROTO_STAT proto_stat;
	uint8_t data_cnt;
	uint8_t data_len;
	uint8_t proto_type;
	uint8_t data_crc;
	uint8_t data[PROTO_DATA_LEN];
	uint32_t head_pos;
	uint32_t tail_pos;
	uint32_t buff_len;
	uint8_t buffer[PROTO_BUFF_LEN];
}PROTO_BUFF;
#endif
/* Global Variable */
uint8_t Intellisys_windown_selected = 0;
uint8_t Intellisys_data_mode_selected = 0;
char master_copy[40] = "Master Data Synced with Kencloud";
char session_copy[40] ="Session Data Synced with Kencloud";
int md = 0;
int sd = 0;
int serialfd;
int serialrecv = 1;
uint8_t upgrade_lock = 0;
unsigned char SerailBuf[READBUFSIZE + 1];
// Select new connect
int fd_array[MAXLINENO];    // accepted connection fd
int conn_amount;    // current connection amount
int array_count;
PROTO_BUFF g_proto_buff;
uint8_t bat_status;
struct tm synctime;


void compare_battery_status(uint8_t received_status)
{
	uint32_t battery_status = (uint32_t)received_status;
	switch ( battery_status)
	{
	case BAT_STATUS_CHARGING:
		printf("charging!\n");
		break;
	case BAT_STATUS_CHARGING_75:
		printf("Battery level : 75%%, charging.\n");
		mlx_proto_led_ctrl(1, LED_YELLOW, 2);
		break;
	case BAT_STATUS_CHARGING_50:
		printf("Battery level : 50%%, charging.\n");
		mlx_proto_led_ctrl(1, LED_YELLOW, 2);
		break;
	case BAT_STATUS_CHARGING_25:
		printf("Battery level : 25%%, charging.\n");
		mlx_proto_led_ctrl(1, LED_YELLOW, 2);
		break;
	case BAT_STATUS_CHARGING_0:
		printf("Battery level : 0%%, charging.\n");
		mlx_proto_led_ctrl(1, LED_YELLOW, 2);
		break;
	case BAT_STATUS_PERCENT_100:
		printf("Battery level : 100%%\n");
		mlx_proto_led_ctrl(1, LED_YELLOW, 0);
		break;
	case BAT_STATUS_PERCENT_75:
		printf("Battery level : 75%%\n");
		mlx_proto_led_ctrl(1, LED_YELLOW, 0);
		break;
	case BAT_STATUS_PERCENT_50:
		printf("Battery level : 50%%\n");
		mlx_proto_led_ctrl(1, LED_YELLOW, 0);
		break;
	case BAT_STATUS_PERCENT_25:
		printf("Battery level : 25%%\n");
		mlx_proto_led_ctrl(1, LED_YELLOW, 0);
		break;
	case BAT_STATUS_PERCENT_0:
		printf("Battery level : 0%%\n");
		mlx_proto_led_ctrl(1, LED_YELLOW, 0);
		break;
	default:
		printf("Invalid value.\n");
		break;
	}
}

void iap_test(const uint8_t *file_path)
{
	int32_t num = 0;
	uint8_t proto_serial[50] = {0};

	mlx_proto_update();

	sleep(8);

	num = locate_module_by_usb_port(MLX_MODULE_STM32);
	if (num < 0)
		return -1;

	sprintf(proto_serial, MLX_PROTO_SERIAL, num);
	printf("%s\n", proto_serial);

	ymodem_send_file(proto_serial, file_path);
}


static void proto_reset(PROTO_BUFF *proto_buff)
{
	if (proto_buff == NULL)
		return;

	proto_buff->proto_stat = PROTO_WAIT_FF;
	proto_buff->data_cnt = 0;
	proto_buff->data_len = 0;
	proto_buff->proto_type = 0;
	proto_buff->data_crc = 0;
	memset(proto_buff->data, 0, sizeof(proto_buff->data));
}

static void insert_byte_to_buffer(PROTO_BUFF *proto_buff, uint8_t byte)
{
	if (proto_buff == NULL)
		return;

	proto_buff->buffer[proto_buff->head_pos] = byte;

	proto_buff->head_pos++;
	if (proto_buff->head_pos >= PROTO_BUFF_LEN)
		proto_buff->head_pos = 0;

	proto_buff->buff_len++;
}

static void handle_proto_data(PROTO_BUFF *proto_buff, uint8_t *buff_value)
{
	if (proto_buff->proto_type & 0x80)
	{
		switch (proto_buff->proto_type & 0x7F)
		{
			case MLX_PROTO_GET_BATTERY_STATUS:
			{
				uint8_t bat_status = *(proto_buff->data);
				uint8_t buff[100] = {0};
				//sprintf(buff, "Battery status is %02x\n", bat_status);
				printf("Battery status is %02x\n", bat_status);
				//mlx_proto_change_rolling_text( 1, buff); 
				//mlx_proto_change_rolling_text( 2, " ");
				//mlx_proto_change_rolling_text( 3, " ");
				break;
			}
			case MLX_PROTO_SYSTEM_RESET:
			{
				break;
			}

			default:
				break;
		}
	}
	else
	{
		switch (proto_buff->proto_type)
		{
			case MLX_PROTO_GET_BATTERY_STATUS:
			{
				bat_status = *(proto_buff->data);
				memcpy( buff_value, proto_buff->data, sizeof(char));
				printf("buff_value is %02x\n", buff_value);
				uint8_t buff[100] = {0};
				//sprintf(buff, "Battery status is %02x\n", bat_status);
				//mlx_proto_change_rolling_text( 1, buff);
				//mlx_proto_change_rolling_text( 2, " ");
				//mlx_proto_change_rolling_text( 3, " ");
				break;
			}
			case MLX_PROTO_GET_ICON_BUTTON_SELECT:
			{
				uint8_t num = *(proto_buff->data);
				uint8_t buff[100] = {0};

				if(Intellisys_windown_selected == 1)
				{
					if(Intellisys_data_mode_selected == 1)
						{
							if( num == 0)
							{
								mlx_proto_change_rolling_text( 1, " ");
								sleep(1);
								mlx_proto_change_rolling_text( 2, " ");
								sleep(1);
								mlx_proto_change_rolling_text( 3, " ");
								sleep(1);
								system("MD_Download");

								//sprintf(buff, "MD button is pressed, %d.\n", num+1);
								//printf("MD button is pressed.Do something...\n");
								
								
							}
							else if ( num == 1)
							{
								mlx_proto_change_rolling_text( 1, " ");
								sleep(1);
								mlx_proto_change_rolling_text( 3, " ");
								sleep(1);
								system("SD_Download");

								//sprintf(buff, "SD button is pressed, %d.\n", num+1);
								//printf("SD button is pressed.Do something...\n");
								
							}
							else if ( num == 2)
							{
								sprintf(buff, "Enter User ID-Principal\n", num+1);
								mlx_proto_change_rolling_text( 1, " ");
								sleep(1);
								mlx_proto_change_rolling_text( 1, buff);
								sleep(3);
								mlx_proto_change_rolling_text( 2, " ");
								sleep(1);
								mlx_proto_change_rolling_text( 3, " ");
								sleep(1);
								mlx_proto_gui_init(GUI_WINDOW_BUTTON_USER_INPUT);
								sleep(2);

								//printf("AUTH button is pressed.Do something...\n");
								//system("./mlx_fingerprint_test");
										
						
							}
							else if(num == 3)
							{
								
								mlx_proto_change_rolling_text( 1, " ");
								sleep(1);
								mlx_proto_change_rolling_text( 2, " ");
								sleep(1);
								mlx_proto_change_rolling_text( 3, " ");
								sleep(1);
								mlx_proto_change_rolling_text( 2, "SOS Sent to Authorised Person");
								sleep(2);
								mlx_proto_change_status_icon(GUI_STATUS_ICON_NOTIFICATION);
								sleep(3);
								

								//sprintf(buff, "Valid IntelliSYS\n", num+1);
								//printf("SOS button is pressed.Do something...\n");

							}
						}
						
					break;
				}	
				else
				{
					printf("nothing\n");
				}
				mlx_proto_change_status_icon(GUI_STATUS_ICON_NOTIFICATION);
				sleep(3);
				//mlx_proto_change_rolling_text( 1, buff);
				//mlx_proto_change_rolling_text( 2, " ");
				//mlx_proto_change_rolling_text( 3, " ");

				
			}
			case MLX_PROTO_GET_USER:
			{
				uint8_t *u = proto_buff->data;
				uint8_t buff[100] = { 0};
				uint8_t id[6] = { 1,0,1,0,1,0};
				sprintf(buff, "The user ID you input is %d%d%d%d%d%d\n", u[0], u[1], u[2], u[3], u[4], u[5]);
				printf("The user ID you input is %d%d%d%d%d%d\n", u[0], u[1], u[2], u[3], u[4], u[5]);
				//mlx_proto_change_rolling_text(2, buff);
				if(strncmp( id, u, 6)==0)
				{
					sprintf(buff, "Valid User ID-Principal\n");
					printf("The user ID is valid\n");
					
					mlx_proto_change_rolling_text(1," ");
					mlx_proto_change_rolling_text(1,buff);
					sleep(2);
					mlx_proto_change_status_icon(GUI_STATUS_ICON_RIGHT);
					sleep(3);
					mlx_proto_change_status_icon(GUI_STATUS_ICON_NOTIFICATION);
					sleep(3);
					
					sprintf(buff, "Enter Pass Code-Principal\n");
					mlx_proto_change_rolling_text(1," ");
					sleep(1);
					mlx_proto_change_rolling_text(1,buff);
					sleep(2);
					mlx_proto_change_rolling_text(2," ");
					mlx_proto_change_rolling_text(3," ");
					mlx_proto_gui_init(GUI_WINDOW_BUTTON_PASSWORD_INPUT);
					sleep(1);
				}
				else
				{
					sprintf(buff, "Invalid User ID-Principal\n");
					printf("The user ID is invalid\n");
					
					mlx_proto_change_rolling_text(1," ");
					sleep(1);
					mlx_proto_change_rolling_text(1,buff);
					sleep(1);
					mlx_proto_change_status_icon(GUI_STATUS_ICON_WRONG);
					sleep(3);
					mlx_proto_change_status_icon(GUI_STATUS_ICON_NOTIFICATION);
					sleep(3);
					
					sprintf(buff, "Enter User ID-Principal\n");
					mlx_proto_change_rolling_text( 1," ");
					sleep(1);
					mlx_proto_change_rolling_text( 1,buff);
					sleep(2);
					mlx_proto_change_rolling_text(2," ");
					mlx_proto_change_rolling_text(3," ");
					mlx_proto_gui_init(GUI_WINDOW_BUTTON_USER_INPUT);
					sleep(1);
				}
				break;
			}
			case MLX_PROTO_GET_PASSWORD:
			{
				uint8_t *p = proto_buff->data;
				uint8_t buff[100] = {0};
				uint8_t pwd[6] = {1,0,1,0,1,0};

				sprintf(buff, "The password you input is %d%d%d%d%d%d\n", p[0], p[1], p[2], p[3], p[4], p[5]);
				printf( "The password you input is %d%d%d%d%d%d\n", p[0], p[1], p[2], p[3], p[4], p[5]);
				//mlx_proto_change_rolling_text(2, buff);
							
				if(strncmp( pwd, p, 6)==0)
				{
					sprintf(buff, "Authentication Successful\n");
					printf("The Pass Code is valid.\n");
					
					mlx_proto_change_rolling_text( 1, " ");
					mlx_proto_change_rolling_text(1, buff);
					sleep(2);
					mlx_proto_change_status_icon(GUI_STATUS_ICON_RIGHT);
					sleep(3);
					mlx_proto_change_rolling_text( 1, " ");
					sleep(1);
					mlx_proto_change_status_icon(GUI_STATUS_ICON_CLEAR);
					sleep(3);					
					mlx_proto_gui_exit(GUI_WINDOW_BUTTON_PASSWORD_INPUT);
					sleep(1);
					if(md == 1 && sd ==1 )
					{
						mlx_proto_change_rolling_text(2," ");
						sleep(1);
						mlx_proto_change_rolling_text(2,master_copy);
						sleep(1);
						mlx_proto_change_rolling_text(3," ");
						sleep(1);						
						mlx_proto_change_rolling_text(3,session_copy);
						sleep(1);
					}
					
					//mlx_proto_change_rolling_text( 1, "SOS Raised Successfully");
					//mlx_proto_change_rolling_text( 2, "If not Getting Help");
					//mlx_proto_change_rolling_text( 3, "Contact:1800 555 5555");
				}
				else
				{
					sprintf(buff, "Invalid Pass Code-Principal\n");
					printf( "The Pass Code is invalid\n");
					
					mlx_proto_change_rolling_text( 1, " ");
					mlx_proto_change_rolling_text( 1, buff);
					sleep(2);
					mlx_proto_change_status_icon(GUI_STATUS_ICON_WRONG);
					sleep(3);
					mlx_proto_change_rolling_text( 1, " ");
					mlx_proto_change_status_icon(GUI_STATUS_ICON_CLEAR);
					sleep(3);
					
					sprintf(buff, "Enter Pass Code-Principal\n");
					mlx_proto_change_rolling_text( 1, buff);
					sleep(2);
					mlx_proto_change_rolling_text( 2, " ");
					mlx_proto_change_rolling_text( 3, " ");
					mlx_proto_gui_init(GUI_WINDOW_BUTTON_PASSWORD_INPUT);
					sleep(1);
				}
				break;
			}
			case MLX_PROTO_GET_SYS_TIME:
			{
				uint8_t *p = (MLX_TIME *)proto_buff->data;
				uint8_t buff[100] = {0};
				MLX_TIME rtc_time;

				
				memset( &rtc_time, 0, sizeof(MLX_TIME));				
				memcpy( &rtc_time, p, sizeof(MLX_TIME));

				synctime.tm_year = MSB_LSB_SWAP16(rtc_time.year) - 1900;
				synctime.tm_mon = rtc_time.month - 1;
				synctime.tm_mday = rtc_time.day;
				synctime.tm_hour = rtc_time.hour;
				synctime.tm_min = rtc_time.minute;
				synctime.tm_sec = rtc_time.second;
                printf( "Year:%04d,mon:%d,day:%d,hour:%d,min:%d,sec:%d\n", MSB_LSB_SWAP16(rtc_time.year), rtc_time.month, rtc_time.day, rtc_time.hour, rtc_time.minute, rtc_time.second);

				//printf("%d-%d-%d %d:%d:%d\n", synctime.tm_year+1900, synctime.tm_mon+1, synctime.tm_mday, synctime.tm_hour, synctime.tm_min, synctime.tm_sec);
				break;
			}
			default:
				break;
		}
	}
}


static void _proto_parse(PROTO_BUFF *proto_buff, uint8_t data, uint8_t *buff_value)
{
    switch (proto_buff->proto_stat)
    {
        case PROTO_WAIT_FF:
        {
            if (data == PROTO_SOH_FF)
            {
            	proto_buff->proto_stat = PROTO_WAIT_55;
            }
            break;
        }
        case PROTO_WAIT_55:
        {
            if (data == PROTO_SOH_55)
            {
            	proto_buff->proto_stat = PROTO_WAIT_AA;
            }
            else
            {
                goto proto_reset;
            }
            break;
        }
        case PROTO_WAIT_AA:
        {
            if (data == PROTO_SOH_AA)
            {
            	proto_buff->proto_stat = PROTO_WAIT_LEN;
            }
            else
            {
                goto proto_reset;
            }
            break;
        }
        case PROTO_WAIT_LEN:
        {
        	proto_buff->data_len = data;
        	proto_buff->data_crc = data;
            proto_buff->proto_stat = PROTO_WAIT_TYPE;
            break;
        }
        case PROTO_WAIT_TYPE:
        {
            if (proto_buff->data_len > proto_buff->data_cnt)
            {
            	proto_buff->data_crc += data;
                proto_buff->data_cnt++;
                proto_buff->proto_type = data;
                if (proto_buff->proto_type & 0x80)
                {
                	proto_buff->proto_stat = PROTO_WAIT_CRC;
                }
                else
                {
                	proto_buff->proto_stat = PROTO_WAIT_DATA;
                }

            }
            break;
        }
        case PROTO_WAIT_DATA:
        {
            if (proto_buff->data_len > proto_buff->data_cnt)
            {
            	proto_buff->data_crc += data;
            	proto_buff->data[proto_buff->data_cnt-1] = data;
                proto_buff->data_cnt++;
                if (proto_buff->data_len == proto_buff->data_cnt)
                {
                	proto_buff->proto_stat = PROTO_WAIT_CRC;
                }
            }
            break;
        }
        case PROTO_WAIT_CRC:
        {
            if (proto_buff->data_crc == data)
            {
            	handle_proto_data(proto_buff, buff_value);
            }
            goto proto_reset;
        }
        default:
        {
            goto proto_reset;
        }
    }

    return;

	proto_reset:
	proto_reset(&g_proto_buff);
}

static void proto_parse_test(PROTO_BUFF *proto_buff)
{
	int32_t fd;
	int32_t num = 0;
	uint32_t i;
	uint32_t recv_len = 0;
	uint8_t proto_serial[50] = {0};
	uint8_t proto_data[100] = {0};
	uint8_t proto_data_value[100] = {0};
	uint32_t buff_len = 0;

	num = locate_module_by_usb_port(MLX_MODULE_STM32);
	if (num < 0)
		return -1;

	sprintf(proto_serial, MLX_PROTO_SERIAL, num);
	printf("%s\n\n", proto_serial);
	fd = s_serial_open(proto_serial);
	if (fd < 0)
	{
		printf("can not open %s\n", proto_serial);
		return -1;
	}

	while (1)
	{
		memset(proto_data, 0, sizeof(proto_data));
		recv_len = zl_serial_read(fd, proto_data, sizeof(proto_data), sizeof(proto_data), 0, ZL_NEVER_MIND);
		if (recv_len)
		{
			printf("receive data: %d\n", recv_len);
			for (i = 0; i < recv_len; i++)
				insert_byte_to_buffer(proto_buff, proto_data[i]);
		}

		buff_len = proto_buff->buff_len;

		if (0 == buff_len)
			continue;

		for (i = 0; i < buff_len; i++)
		{
			_proto_parse(proto_buff, proto_buff->buffer[proto_buff->tail_pos], &proto_data_value);
			proto_buff->tail_pos++;
			if (proto_buff->tail_pos >= PROTO_BUFF_LEN)
				proto_buff->tail_pos = 0;
		}

		proto_buff->buff_len -= buff_len;

		usleep(1000*10);
	}
}

#if 0
void *pthread_client_do(void *arg) 
{
	Serailbuff Sbuff;
	Sbuff.fd = ((Serailbuff *) arg)->fd;
	Sbuff.size = ((Serailbuff *) arg)->size;
	memcpy(Sbuff.data, ((Serailbuff *) arg)->data, Sbuff.size);
	send(Sbuff.fd, Sbuff.data, Sbuff.size, 0);
	Sleep(0, 500);
	return (void *) NULL;
}
#endif
void pthread_battery_check(void *arg)
{
    uint8_t status;
    int32_t flag = 0;
    while(1)
    {
        //printf("while loop\n");
        mlx_proto_get_battery_status( &status, 1000);
        compare_battery_status( bat_status);
        printf("\n");
        sleep(3);
    }
    return (void *) NULL;
}

void *pthread_event_uart(void *arg) {
	int retval;
	uint8_t proto_data[100] = {0};
	uint8_t proto_data_value[100] = {0};
	PROTO_BUFF *proto_buff;
	uint32_t buff_len = 0;
	struct timeval tv;
	fd_set fdsetrd;
	int maxfds;
	FD_ZERO(&fdsetrd);
	FD_SET(serialfd, &fdsetrd);
	maxfds = serialfd;

	proto_buff = (PROTO_BUFF *)malloc(sizeof(PROTO_BUFF));
	while (1) {
		// initialize file descriptor set
		FD_ZERO(&fdsetrd);
		FD_SET(serialfd, &fdsetrd);
		// timeout setting
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		retval = select(maxfds + 1, &fdsetrd, NULL, NULL, &tv);
		if (retval < 0) {
			break;
		} else if (retval == 0) {
			continue;
		}
		if (FD_ISSET(serialfd, &fdsetrd)) {
			unsigned char serialData[READBUFSIZE];
			bzero(serialData, READBUFSIZE);
			unsigned char *pbuf = serialData;
			int remain = READBUFSIZE;
			int total = READBUFSIZE;
			int readBytes = 0;
			memset(proto_data, 0, sizeof(proto_data));
			readBytes = zl_serial_read(serialfd, proto_data, sizeof(proto_data),
					sizeof(proto_data), 100, ZL_NEVER_MIND);
			printf("receive data: %d\n", readBytes);
			int y = 0;
			for (y = 0; y < readBytes; y++)
			{
				printf("%02x ", proto_data[y]);
			}
			printf("\n");

			if (proto_data[0] == 0xFF && proto_data[1] == 0x55)
			{
				int i = 0;
				memset(proto_buff, 0, sizeof(PROTO_BUFF));
				if (readBytes)
				{
					for (i = 0; i < readBytes; i++)
						insert_byte_to_buffer(proto_buff, proto_data[i]);
				}

				buff_len = proto_buff->buff_len;

				if (0 == buff_len)
					continue;

				for (i = 0; i < buff_len; i++)
				{
					_proto_parse(proto_buff, proto_buff->buffer[proto_buff->tail_pos], &proto_data_value);
					proto_buff->tail_pos++;
					if (proto_buff->tail_pos >= PROTO_BUFF_LEN)
						proto_buff->tail_pos = 0;
				}

				proto_buff->buff_len -= buff_len;

#if 0
				pthread_t pt_client;
				pthread_attr_t pt_client_attr;
				pthread_attr_init(&pt_client_attr);
				pthread_attr_setdetachstate(&pt_client_attr,
				PTHREAD_CREATE_DETACHED);
				pthread_create(&pt_client, &pt_client_attr,
						pthread_client_do, &data);
#endif

				Sleep(0, 20000);
			} else {
				tcflush(serialfd, TCIOFLUSH);
			}
		}
		Sleep(0, 50 * 1000);
	}
	return (void *) NULL;
}


int main(int argc , char ** argv[])
{
    	uint32_t ret;
	int32_t num = 0;
	uint8_t proto_serial[50] = {0};
	char time_s[30];//ch
	pthread_t uart_recv_do;
        pthread_t battery_check_do;
	int time_count = 0;//ch
	struct sigaction sa;
	sa.sa_handler = SIG_IGN;
	sa.sa_flags = 0;
	if (sigemptyset(&sa.sa_mask) == -1 || sigaction(SIGPIPE, &sa, 0) == -1) {
		return -1;
	}

	num = locate_module_by_usb_port(MLX_MODULE_STM32);
	if (num < 0)
		return -1;

	sprintf(proto_serial, MLX_PROTO_SERIAL, num);
	printf("%s\n\n", proto_serial);
	serialfd = s_serial_open(proto_serial);
	if (serialfd < 0)
	{
		printf("can not open %s\n", proto_serial);
		return -1;
	}


	pthread_create(&uart_recv_do, NULL, pthread_event_uart, NULL);
        pthread_create(&battery_check_do, NULL, pthread_battery_check, NULL);
	mlx_proto_system_reset();

/*###############################################################################################################################################*/
	
					Intellisys_windown_selected = 1;
					Intellisys_data_mode_selected = 1;
					mlx_proto_gui_init(GUI_WINDOW_LOGO_INTELLISYS);
					sleep(3);
					mlx_proto_gui_init(GUI_WINDOW_MAIN_INTELLISYS);
					sleep(3);
					mlx_proto_gui_init(GUI_WINDOW_BUTTON_INTELLISYS);
					sleep(3);
					system("wanconnect");
					sleep(2);
					mlx_proto_get_sys_time();
					sleep(2);
					sprintf(time_s,"date -s '%d-%d-%0d %d:%d:%d'",synctime.tm_year+1900, synctime.tm_mon+1, synctime.tm_mday, synctime.tm_hour, synctime.tm_min, synctime.tm_sec);
					printf("Time String: %s\n",time_s);
					system(time_s);	
					sleep(2);
						mlx_proto_change_rolling_text(1," ");
						mlx_proto_gui_init(GUI_WINDOW_BUTTON_INTELLISYS);
						sleep(3);
						
						system("MD_Download");
						sleep(2);
						md =1;
						system("SD_Download");	
						sleep(2);
						sd = 1;
						system("mlx_fingerprint_test");
						sleep(2);
						
			                        //system("chmod +x ssid.py");
						//system("python ssid.py");
                                             
						mlx_proto_change_status_icon(GUI_STATUS_ICON_CLEAR);
						sleep(3);
						mlx_proto_change_rolling_text(1," ");	
						sleep(1);

					while(1)
    						{
        						//printf("while loop\n");
    							//mlx_proto_get_battery_status( &status, 1000);
    							//compare_battery_status( bat_status);
    							//printf("\n");
							mlx_proto_get_sys_time();
							sleep(2);
        						printf("%d-%d-%d %d:%d:%d\n", synctime.tm_year+1900, synctime.tm_mon+1, synctime.tm_mday, synctime.tm_hour, synctime.tm_min, synctime.tm_sec);
							
							printf("Example: \nwe use a global variable \"struct tm synctime\" stored the rtc time on line 136.\n");
							printf("Synctime is time structure \"struct tm\" which is system structure.\n");
							printf("We can use synctime to compare.");
							printf("\n");
							sprintf(time_s,"date -s '%d-%d-%0d %d:%d:%d'",synctime.tm_year+1900, synctime.tm_mon+1, synctime.tm_mday, synctime.tm_hour, synctime.tm_min, synctime.tm_sec);
							printf("Time String: %s\n",time_s);
							system(time_s);	
							//time_count++;

       							sleep(10);

    						}

						

	
	
/*####################################################################################################################################################*/	
	
        return EXIT_SUCCESS;
}


