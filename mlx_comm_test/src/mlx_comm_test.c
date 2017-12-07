#include <stdio.h>
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

#define MSB_LSB_SWAP16(A)     ((((u_int16_t)(A) & 0xff00) >> 8) | \
                    (((u_int16_t)(A) & 0x00ff) << 8))

#define MLX_COMM_TEST_HELP "\n\n\
Usage: mlx_comm_test option [param 1th] [param 2th] [param 3th]\n\
\toption\tparam\t\t\t\tex.\t\t\t\t\t\tusage\n\
\t-r\tN/A\t\t\t\tmlx_comm_test -r\t\t\t\tReboot the system\n\n\
\t-g\tN/A\t\t\t\tmlx_comm_test -g\t\t\t\tGet system RTC time\n\n\
\t-u\tpath of update file\t\tmlx_comm_test -u /tmp/IAP_TEST.bin\t\tUpadte the system using a update file\n\n\
\t-p\tN/A\t\t\t\tmlx_comm_test -p\t\t\t\tA protol parse demo\n\n\
\t-w\tBelow\t\t\t\tBelow\t\t\t\t\t\tInit and create the window\n\
\t\tsys_logo\t\t\tmlx_comm_test -w sys_logo\t\t\tShow itelliSYS logo icon\n\
\t\tsense_logo\t\t\tmlx_comm_test -w sense_logo\t\t\tShow itelliSENSE logo icon\n\
\t\tsys_main\t\t\tmlx_comm_test -w sys_main\t\t\tShow itelliSYS main window\n\
\t\tsense_main\t\t\tmlx_comm_test -w sense_main\t\t\tShow itelliSENSE main window\n\
\t\tsys_button\t\t\tmlx_comm_test -w sys_button\t\t\tShow itelliSYS icon button\n\
\t\tsense_select_button\t\tmlx_comm_test -w sense_select_button\t\tShow itelliSENSE select icon button\n\
\t\tsense_ap_button\t\t\tmlx_comm_test -w sense_ap_button\t\tShow itelliSENSE ap mode icon button\n\
\t\tsense_data_button\t\tmlx_comm_test -w sense_data_button\t\tShow itelliSENSE data mode icon button\n\n\
\t-s\tsystem time\t\t\tmlx_comm_test -s 2017-01-01 12:01:03\t\tChange system time to 2017-01-01 12:01:03\n\n\
\t-c\tcollege name\t\t\tmlx_comm_test -c abc\t\t\t\tChange the college name to \"abc\"\n\n\
\t-i\tBelow\t\t\t\tBelow\t\t\t\t\t\tChange the status icon in middle window\n\
\t\tnotify\t\t\t\tmlx_comm_test -i notify\t\t\t\tShow the notification icon\n\
\t\talert\t\t\t\tmlx_comm_test -i alert\t\t\t\tShow the alert icon\n\
\t\tright\t\t\t\tmlx_comm_test -i right\t\t\t\tShow the right icon\n\
\t\twrong\t\t\t\tmlx_comm_test -i wrong\t\t\t\tShow the wrong icon\n\
\t\tfinger\t\t\t\tmlx_comm_test -i finger\t\t\t\tShow the finger icon\n\
\t\tclear\t\t\t\tmlx_comm_test -i clear\t\t\t\tClear the status icon\n\n\
\t-d\tBelow\t\t\t\tBelow\t\t\t\t\t\tShow a demo of GUI\n\
\t\tsys\t\t\t\tmlx_comm_test -d sys\t\t\t\tShow a demo of itelliSYS\n\
\t\tsense_ap\t\t\tmlx_comm_test -d sense_ap\t\t\tShow a demo of itelliSENSE of ap mode\n\
\t\tsense_data\t\t\tmlx_comm_test -d sense_data\t\t\tShow a demo of itelliSENSE of data mode\n\n\
\t-t\tBelow\t\t\t\tBelow\t\t\t\t\t\tShow a string(prarm string) on specified line(prarm line)\n\
\t\tprarm 1th\t\t\t\t\t\t\t\t\tThe first parameter that value should be 1-3 or \"clear\"\n\
\t\tline\t\t\t\tmlx_comm_test -t 1 abc\t\t\t\tShow \"abc\" on 1th line\n\
\t\tprarm 2th\t\t\t\t\t\t\t\t\tThe second parameter. A string you wanted show\n\
\t\tstring\t\t\t\tmlx_comm_test -t 1 clear\t\t\tClear all string on 1th line\n\n\
\t-b\tBelow\t\t\t\tBelow\t\t\t\t\t\tShow the specified icon(prarm icon) on specified position(prarm num) on status bar\n\
\t\tprarm 1th\t\t\t\t\t\t\t\t\tThe first parameter that value should be 1-4\n\
\t\tnum\n\
\t\tprarm 2th\t\t\t\t\t\t\t\t\tThe second parameter. The specified icon you want to show\n\
\t\ticon\n\
\t\tap_mode\t\t\t\tmlx_comm_test -b 1 ap_mode\t\t\tShow the ap mode icon\n\
\t\tdata_mode\t\t\tmlx_comm_test -b 3 data_mode\t\t\tShow the data mode icon\n\
\t\tbt_conn\t\t\t\tmlx_comm_test -b 2 bt_conn\t\t\tShow the bluetooth connected icon\n\
\t\tbt_disconn\t\t\tmlx_comm_test -b 2 bt_disconn\t\t\tShow the bluetooth disconnected icon\n\
\t\teth_conn\t\t\tmlx_comm_test -b 3 eth_conn\t\t\tShow the ethernet connected icon\n\
\t\teth_disconn\t\t\tmlx_comm_test -b 3 eth_disconn\t\t\tShow ethernet disconnected icon\n\
\t\tcloud_conn\t\t\tmlx_comm_test -b 4 cloud_conn\t\t\tShow cloud connected icon\n\
\t\tcloud_disconn\t\t\tmlx_comm_test -b 4 cloud_disconn\t\tShow cloud disconnected icon\n\
\t\tgps_en\t\t\t\tmlx_comm_test -b 2 gps_en\t\t\tShow GPS enabled icon\n\
\t\tgps_dis\t\t\t\tmlx_comm_test -b 2 gps_dis\t\t\tShow GPS disabled icon\n\
\t\tntf_en\t\t\t\tmlx_comm_test -b 3 ntf_en\t\t\tShow NTF enabled icon\n\
\t\tntf_dis\t\t\t\tmlx_comm_test -b 3 ntf_dis\t\t\tShow the NTF disabled icon\n\
\t\twifi_en\t\t\t\tmlx_comm_test -b 4 wifi_en\t\t\tShow WiFi enabled icon\n\
\t\twifi_dis\t\t\tmlx_comm_test -b 4 wifi_dis\t\t\tShow WiFi disabled icon\n\n\
\t-l\tBelow\t\t\t\tBelow\t\t\t\t\t\tLED control\n\
\t\tprarm 1th\t\t\t\t\t\t\t\t\tThe first parameter that value should be 1-3\n\
\t\tid\n\
\t\tprarm 2th\t\t\t\t\t\t\t\t\tThe second parameter. The color of led you want to show\n\
\t\tcolor\n\
\t\tyellow\t\t\t\tmlx_comm_test -l 1 yellow on\t\t\tLed1 light with yellow\n\
\t\tred\t\t\t\tmlx_comm_test -l 2 red on\t\t\tLed2 light with red\n\
\t\tgreen\t\t\t\tmlx_comm_test -l 3 green on\t\t\tLed3 light with green\n\
\t\tprarm 3th\t\t\t\t\t\t\t\t\tThe third parameter. The status of led you want to show\n\
\t\tstatus\n\
\t\ton\t\t\t\tmlx_comm_test -l 1 yellow on\t\t\tLed1 light with yellow\n\
\t\toff\t\t\t\tmlx_comm_test -l 2 red off\t\t\tRed color of Led2 turn off\n\
\t\tblink\t\t\t\tmlx_comm_test -l 3 yellow blink\t\t\tYellow color of Led3 blink\n\n\
\t-v\tN/A\t\t\t\tmlx_comm_test -v\t\t\t\tGet system software version\n\n\
\t-h\tN/A\t\t\t\tmlx_comm_test -h\t\t\t\tShow the usage of mlx_comm_test\n\n\
"

#define MLX_PROTO_SERIAL		"/dev/ttyUSB%d,115200,8,N,1"
#define MLX_UPDATE_SERIAL		"/dev/ttyUSB%d"

#define PROTO_BUFF_LEN				2048
#define PROTO_DATA_LEN				256

#define PROTO_SOH_FF    			0xFF
#define PROTO_SOH_55    			0x55
#define PROTO_SOH_AA    			0xAA

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

PROTO_BUFF g_proto_buff;

void iap_test(const uint8_t *file_path)
{
	int32_t num = 0;
	uint8_t proto_serial[50] = {0};
	
	mlx_proto_update();

	sleep(8);

	num = locate_module_by_usb_port(MLX_MODULE_STM32);
	if (num < 0)
		return -1;

	sprintf(proto_serial, MLX_UPDATE_SERIAL, num);
	printf("%s\n", proto_serial);
	
	ymodem_send_file(proto_serial, file_path);
}

void itellisys_test(void)
{
	mlx_proto_system_reset();
	sleep(1);
	
	mlx_proto_gui_init(GUI_WINDOW_LOGO_INTELLISYS);
	sleep(3);
	
	mlx_proto_gui_init(GUI_WINDOW_MAIN_INTELLISYS);
	mlx_proto_change_rolling_text(2, "Network Not Connected ...");
	mlx_proto_change_status_bar_icon(3, GUI_STATUS_ETH_CONN);
	mlx_proto_change_status_bar_icon(4, GUI_STATUS_CLOUD_CONN);
	mlx_proto_change_status_bar_icon(5, GUI_STATUS_GPS_ENABLE);
	sleep(3);

	mlx_proto_change_status_bar_icon(3, GUI_STATUS_ETH_DISCONN);
	mlx_proto_change_status_bar_icon(4, GUI_STATUS_CLOUD_DISCONN);
	mlx_proto_change_status_bar_icon(5, GUI_STATUS_GPS_DISABLE);
	sleep(3);
	
	mlx_proto_change_college_name("Ramaiah Engineering College");
	mlx_proto_change_rolling_text(2, "Invalid IntelliSYS");
	mlx_proto_change_status_icon(GUI_STATUS_ICON_WRONG);
	mlx_proto_gui_init(GUI_WINDOW_BUTTON_INTELLISYS);
	sleep(3);

	mlx_proto_change_rolling_text(1, " ");
	mlx_proto_change_rolling_text(2, "Master Data Download Failed");
	mlx_proto_change_rolling_text(3, " ");
	mlx_proto_change_status_icon(GUI_STATUS_ICON_ALERT);
	sleep(3);

	mlx_proto_change_rolling_text(1, "SOS Raised Successfully");
	mlx_proto_change_rolling_text(2, "If not Getting Help");
	mlx_proto_change_rolling_text(3, "Contact:1800 555 5555");
	mlx_proto_change_status_icon(GUI_STATUS_ICON_NOTIFICATION);
	sleep(3);
	
	//mlx_proto_change_college_name("Ramaiah Engineering College");
	//mlx_proto_change_rolling_text(1, "Enter Pass Code");
	//mlx_proto_change_rolling_text(2, " ");
	//mlx_proto_change_status_icon(GUI_STATUS_ICON_NOTIFICATION);
	//mlx_proto_gui_init(GUI_WINDOW_BUTTON_PASSWORD_INPUT);
	//sleep(3);
	
	mlx_proto_system_reset();
}

void itellisense_data_mode_test(void)
{
	mlx_proto_system_reset();
	sleep(1);
	
	mlx_proto_gui_init(GUI_WINDOW_LOGO_INTELLISENSE);
	sleep(3);
	
	mlx_proto_gui_init(GUI_WINDOW_MAIN_INTELLISENSE);
	mlx_proto_change_rolling_text(2, "Please Select Mode");
	mlx_proto_change_status_icon(GUI_STATUS_ICON_NOTIFICATION);
	mlx_proto_gui_init(GUI_WINDOW_BUTTON_INTELLISENSE_SELECT_MODE);
	sleep(3);

	mlx_proto_change_status_bar_icon(3, GUI_STATUS_DATA_MODE);
	mlx_proto_change_status_bar_icon(4, GUI_STATUS_ETH_CONN);
	mlx_proto_change_status_bar_icon(5, GUI_STATUS_GPS_ENABLE);
	mlx_proto_change_college_name("Ramaiah Engineering College");
	mlx_proto_change_rolling_text(2, "Valid IntelliSENSE");
	mlx_proto_change_status_icon(GUI_STATUS_ICON_RIGHT);
	mlx_proto_gui_init(GUI_WINDOW_BUTTON_INTELLISENSE_DATA_MODE);
	sleep(3);

	mlx_proto_change_status_bar_icon(4, GUI_STATUS_ETH_DISCONN);
	mlx_proto_change_status_bar_icon(5, GUI_STATUS_GPS_DISABLE);
	sleep(3);
	
	mlx_proto_change_rolling_text(1, " ");
	mlx_proto_change_rolling_text(2, "Master Data Sync ...");
	mlx_proto_change_rolling_text(3, " ");
	mlx_proto_change_status_icon(GUI_STATUS_ICON_NOTIFICATION);
	sleep(3);

	mlx_proto_change_rolling_text(1, " ");
	mlx_proto_change_rolling_text(2, "Master Data Sync Failed");
	mlx_proto_change_rolling_text(3, " ");
	mlx_proto_change_status_icon(GUI_STATUS_ICON_ALERT);
	sleep(3);

	mlx_proto_system_reset();

	//mlx_proto_change_college_name("Ramaiah Engineering College");
	//mlx_proto_change_rolling_text(1, "Enter Faculty ID");
	//mlx_proto_change_rolling_text(2, " ");
	//mlx_proto_change_status_icon(GUI_STATUS_ICON_NOTIFICATION);
	//mlx_proto_gui_init(GUI_WINDOW_BUTTON_USER_INPUT);
	//sleep(15);
	
	//mlx_proto_change_status_icon(GUI_STATUS_ICON_ALERT);
	//sleep(3);
}

void itellisense_ap_mode_test(void)
{
	mlx_proto_system_reset();
	sleep(1);
	
	mlx_proto_gui_init(GUI_WINDOW_LOGO_INTELLISENSE);
	sleep(3);
	
	mlx_proto_gui_init(GUI_WINDOW_MAIN_INTELLISENSE);
	mlx_proto_change_rolling_text(2, "Please Select Mode");
	mlx_proto_change_status_icon(GUI_STATUS_ICON_NOTIFICATION);
	mlx_proto_gui_init(GUI_WINDOW_BUTTON_INTELLISENSE_SELECT_MODE);
	sleep(3);

	mlx_proto_change_status_bar_icon(1, GUI_STATUS_AP_MODE);
	mlx_proto_change_status_bar_icon(2, GUI_STATUS_BT_CONN);
	mlx_proto_change_status_bar_icon(3, GUI_STATUS_NTF_ENABLED);
	mlx_proto_change_status_bar_icon(4, GUI_STATUS_WIFI_ENABLED);
	mlx_proto_change_status_bar_icon(5, GUI_STATUS_GPS_ENABLE);	
	mlx_proto_change_college_name("Ramaiah Engineering College");
	mlx_proto_change_rolling_text(2, "Please Authenticate");
	mlx_proto_change_status_icon(GUI_STATUS_ICON_FINGER_PRINT);
	mlx_proto_gui_init(GUI_WINDOW_BUTTON_INTELLISENSE_WIFI_MODE);
	sleep(3);

	mlx_proto_change_status_bar_icon(2, GUI_STATUS_BT_DISCONN);
	mlx_proto_change_status_bar_icon(3, GUI_STATUS_NTF_DISABLED);
	mlx_proto_change_status_bar_icon(4, GUI_STATUS_WIFI_DISABLED);
	mlx_proto_change_status_bar_icon(5, GUI_STATUS_GPS_DISABLE);
	sleep(3);

	mlx_proto_change_rolling_text(1, "Authentication Failed");
	mlx_proto_change_rolling_text(2, "Please Try Again");
	mlx_proto_change_status_icon(GUI_STATUS_ICON_WRONG);
	sleep(3);

	mlx_proto_change_rolling_text(1, "Start Time: 10:10 AM");
	mlx_proto_change_rolling_text(2, "End Time: 12:30 PM");
	mlx_proto_change_rolling_text(3, "Connected IntelliPAD: 30/50");
	mlx_proto_change_status_icon(GUI_STATUS_ICON_NOTIFICATION);
	sleep(3);

	mlx_proto_system_reset();	
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

static void handle_proto_data(PROTO_BUFF *proto_buff)
{
	if (proto_buff->proto_type & 0x80)
	{
		switch (proto_buff->proto_type & 0x7F)
		{
			case MLX_PROTO_GET_BATTERY_STATUS:
			{
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
			case MLX_PROTO_GET_ICON_BUTTON_SELECT:
			{
				uint8_t num = *(proto_buff->data);
				uint8_t buff[100] = {0};
				sprintf(buff, "button %dth is selected\n", num+1);
				mlx_proto_change_rolling_text(1, buff);
				mlx_proto_change_rolling_text(2, " ");
				mlx_proto_change_rolling_text(3, " ");
				
				break;
			}
			case MLX_PROTO_GET_USER:
			{
				uint8_t *p = proto_buff->data;
				uint8_t buff[100] = {0};
				sprintf(buff, "The user ID you input is %d%d%d%d%d%d\n", p[0], p[1], p[2], p[3], p[4], p[5]);
				mlx_proto_change_rolling_text(2, buff);
				break;
			}
			case MLX_PROTO_GET_PASSWORD:
			{
				uint8_t *p = proto_buff->data;
				uint8_t buff[100] = {0};
				sprintf(buff, "The password you input is %d%d%d%d%d%d\n", p[0], p[1], p[2], p[3], p[4], p[5]);
				mlx_proto_change_rolling_text(2, buff);
				break;
			}
			case MLX_PROTO_GET_SYS_TIME:
			{
				uint8_t *p = (MLX_TIME *)proto_buff->data;
				uint8_t buff[100] = {0};
				MLX_TIME rtc_time;
				memset( &rtc_time, 0, sizeof(MLX_TIME));
				memcpy( &rtc_time, p, sizeof(MLX_TIME));
                printf( "Year:%04d,mon:%d,day:%d,hour:%d,min:%d,sec:%d\n", MSB_LSB_SWAP16(rtc_time.year), rtc_time.month, rtc_time.day, rtc_time.hour, rtc_time.minute, rtc_time.second);
				break;
			}
			default:
				break;
		}
	}
}

static void _proto_parse(PROTO_BUFF *proto_buff, uint8_t data)
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
            	handle_proto_data(proto_buff);
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
	uint32_t buff_len = 0;

	num = locate_module_by_usb_port(MLX_MODULE_STM32);
	if (num < 0)
		return -1;
	
	sprintf(proto_serial, MLX_PROTO_SERIAL, num);
	printf("%s\n\n", proto_serial);
	fd = s_serial_open(proto_serial);//\B4򿪴\AE\BF\DA  
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
			_proto_parse(proto_buff, proto_buff->buffer[proto_buff->tail_pos]);
			proto_buff->tail_pos++;
			if (proto_buff->tail_pos >= PROTO_BUFF_LEN)
				proto_buff->tail_pos = 0;
		}

		proto_buff->buff_len -= buff_len;

		usleep(1000*10);
	}
}

int32_t main (int argc, char *argv[])
{
	int32_t ch;
	int32_t num = 0;
	int32_t	freq = 0;
	const uint8_t *text = NULL;
	MLX_TIME time;
	MLX_STATUS_BAR_ICON icon;
	MLX_LED_COLOR color;
	uint8_t *minute = NULL; 
	uint8_t *second = NULL; 
	uint8_t *hour = NULL; 
	uint8_t *day= NULL; 
	uint8_t *month= NULL;
	uint8_t *year = NULL;
	
	if (argc < 2)
		goto err;
		
	while((ch = getopt(argc, argv, "rgphvw:s:c:i:t:d:u:b:l:")) != EOF)
	{
		switch(ch)
		{
			case 'r': 
				if (argc != 2)
					goto err;
				mlx_proto_system_reset();
				break; 
			case 'g': 
				if (argc != 2)
					goto err;
				mlx_proto_get_sys_time();
				break; 
			case 'v': 
				if (argc == 2) 
					mlx_proto_get_sw_version();
				else if (argc == 3)
					mlx_proto_set_sw_version(argv[2]);
				else
					goto err;
				break; 
			case 'p':
				if (argc != 2)
					goto err;
				proto_parse_test(&g_proto_buff);
				break;
			case 'w': 
				if (argc != 3)
					goto err;

				if (strcmp(argv[2], "sys_logo") == 0)
					mlx_proto_gui_init(GUI_WINDOW_LOGO_INTELLISYS);
				else if (strcmp(argv[2], "sense_logo") == 0)
					mlx_proto_gui_init(GUI_WINDOW_LOGO_INTELLISENSE);
				else if (strcmp(argv[2], "sys_main") == 0)
					mlx_proto_gui_init(GUI_WINDOW_MAIN_INTELLISYS);
				else if (strcmp(argv[2], "sense_main") == 0)
					mlx_proto_gui_init(GUI_WINDOW_MAIN_INTELLISENSE);
				else if (strcmp(argv[2], "sys_button") == 0)
					mlx_proto_gui_init(GUI_WINDOW_BUTTON_INTELLISYS);
				else if (strcmp(argv[2], "sense_select_button") == 0)
					mlx_proto_gui_init(GUI_WINDOW_BUTTON_INTELLISENSE_SELECT_MODE);
				else if (strcmp(argv[2], "sense_ap_button") == 0)
					mlx_proto_gui_init(GUI_WINDOW_BUTTON_INTELLISENSE_WIFI_MODE);
				else if (strcmp(argv[2], "sense_data_button") == 0)
					mlx_proto_gui_init(GUI_WINDOW_BUTTON_INTELLISENSE_DATA_MODE);
				else if (strcmp(argv[2], "sense_user") == 0)
					mlx_proto_gui_init(GUI_WINDOW_BUTTON_USER_INPUT);
				else if (strcmp(argv[2], "sense_pwd") == 0)
					mlx_proto_gui_init(GUI_WINDOW_BUTTON_PASSWORD_INPUT);
				else if (strcmp(argv[2], "lcd_test") == 0)
					mlx_proto_gui_init(GUI_WINDOW_FACTORY_LCD_TEST);
				else if (strcmp(argv[2], "key_test") == 0)
					mlx_proto_gui_init(GUI_WINDOW_FACTORY_KEY_TEST);
				else 
					goto err;
				break;
			case 's': 
				if (argc != 4)
					goto err;

				hour = strtok(argv[3],":");
				if (hour == NULL)
					return;

				minute = strtok(NULL,":");
				if (minute == NULL)
					return;

				second = strtok(NULL,":");
				if (second == NULL)
					return;

				year = strtok(argv[2], "-");
				if (year == NULL)
					return;

				month = strtok(NULL, "-");
				if (month == NULL)
					return;

				day = strtok(NULL, "-");
				if (day == NULL)
					return;

				time.second = atoi(second);
				time.minute = atoi(minute);
				time.hour = atoi(hour);
				time.day = atoi(day);
				time.month = atoi(month);
				time.year = atoi(year);
				
				mlx_proto_change_system_time(&time);
				break; 
			case 'c': 
				if (argc != 3)
					goto err;
				
				mlx_proto_change_college_name(argv[2]);
				break; 
			case 'i': 
				if (argc != 3)
					goto err;
				
				if (strcmp(argv[2], "notify") == 0)
					mlx_proto_change_status_icon(GUI_STATUS_ICON_NOTIFICATION);
				else if (strcmp(argv[2], "alert") == 0)
					mlx_proto_change_status_icon(GUI_STATUS_ICON_ALERT);
				else if (strcmp(argv[2], "right") == 0)
					mlx_proto_change_status_icon(GUI_STATUS_ICON_RIGHT);
				else if (strcmp(argv[2], "wrong") == 0)
					mlx_proto_change_status_icon(GUI_STATUS_ICON_WRONG);
				else if (strcmp(argv[2], "finger") == 0)
					mlx_proto_change_status_icon(GUI_STATUS_ICON_FINGER_PRINT);
				else if (strcmp(argv[2], "clear") == 0)
					mlx_proto_change_status_icon(GUI_STATUS_ICON_CLEAR);
				else 
					printf("illegal option\n");
				break; 
			case 't':
				if (argc != 4)
					goto err;

				if (strcmp(argv[3], "clear") == 0)
					text = " ";
				else 
					text = argv[3];
				
				num = atoi(argv[2]);
				if ((num < 1) || (num > 3))
					goto err;
				mlx_proto_change_rolling_text(num, text);
				break; 
			case 'b':
				if (argc != 4)
					goto err;

				num = atoi(argv[2]);
				if ((num < 1) || (num > 5))
					goto err;
				
				if (strcmp(argv[3], "ap_mode") == 0)
					icon = GUI_STATUS_AP_MODE;
				else if (strcmp(argv[3], "data_mode") == 0)
					icon = GUI_STATUS_DATA_MODE;
				else if (strcmp(argv[3], "bt_conn") == 0)
					icon = GUI_STATUS_BT_CONN;
				else if (strcmp(argv[3], "bt_disconn") == 0)
					icon = GUI_STATUS_BT_DISCONN;
				else if (strcmp(argv[3], "eth_conn") == 0)
					icon = GUI_STATUS_ETH_CONN;
				else if (strcmp(argv[3], "eth_disconn") == 0)
					icon = GUI_STATUS_ETH_DISCONN;
				else if (strcmp(argv[3], "cloud_conn") == 0)
					icon = GUI_STATUS_CLOUD_CONN;
				else if (strcmp(argv[3], "cloud_disconn") == 0)
					icon = GUI_STATUS_CLOUD_DISCONN;	
				else if (strcmp(argv[3], "gps_en") == 0)
					icon = GUI_STATUS_GPS_ENABLE;
				else if (strcmp(argv[3], "gps_dis") == 0)
					icon = GUI_STATUS_GPS_DISABLE;		
				else if (strcmp(argv[3], "bt_en") == 0)
					icon = GUI_STATUS_NTF_ENABLED;
				else if (strcmp(argv[3], "bt_dis") == 0)
					icon = GUI_STATUS_NTF_DISABLED;
				else if (strcmp(argv[3], "wifi_en") == 0)
					icon = GUI_STATUS_WIFI_ENABLED;
				else if (strcmp(argv[3], "wifi_dis") == 0)
					icon = GUI_STATUS_WIFI_DISABLED;
				else 
					goto err;
				
				mlx_proto_change_status_bar_icon(num, icon);
				break; 
			case 'd':
				if (argc != 3)
					goto err;
			
				if (strcmp(argv[2], "sys") == 0)
					itellisys_test();
				else if (strcmp(argv[2], "sense_ap") == 0)
					itellisense_ap_mode_test();
				else if (strcmp(argv[2], "sense_data") == 0)
					itellisense_data_mode_test();
				else 
					printf("illegal option\n");
				break;
			case 'u':
				if (argc != 3)
					goto err;

				if (gen_check_file_exist(argv[2]))
					iap_test(argv[2]);
				else
					printf("update file does not exist!!!\n");	
				break;
			case 'l':
				if (argc != 5)
					goto err;

				num = atoi(argv[2]);
				if ((num < 1) || (num > 3))
					goto err;

				if (strcmp(argv[3], "yellow") == 0)
					color = LED_YELLOW;
				else if (strcmp(argv[3], "red") == 0)
					color = LED_RED;
				else if (strcmp(argv[3], "green") == 0)
					color = LED_GREEN;
				else
					goto err;

				if (strcmp(argv[4], "on") == 0)
					freq = 1;
				else if (strcmp(argv[4], "off") == 0)
					freq = 0;
				else if (strcmp(argv[4], "blink") == 0)
					freq = 2;
				else
					goto err;
				
				mlx_proto_led_ctrl(num, color, freq);
				break;
			case 'h':
				if (argc != 2)
					goto err;
				printf(MLX_COMM_TEST_HELP);
				break;
			default:
				goto err;
		}
	}

	return 0;

err:
	printf("illegal option\n");
	return -1;
	
}
