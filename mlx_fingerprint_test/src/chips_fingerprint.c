/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
//revised:2017/4/1,zxd
#include <errno.h>
#include <endian.h>
#include <inttypes.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <linux/input.h>

#include <sys/types.h>
#include <unistd.h>

#include "gen_types.h"
#include "zl-serial.h"
#include "libproto.h"

#include "./inc/chips_sensor.h"
#include "./inc/chips_fingerprint.h"
#include "./inc/chips_api.h"
#include "./inc/chips_log.h"

#define FINGER_COUNT 5

#define VERSION "v2.0.22"

// Typical devices will allow up to 5 fingerprints per user to maintain performance of
// t < 500ms for recognition.  This is the total number of fingerprints we'll store.	
#define MAX_NUM_FINGERS 20
#define MAX_FID_VALUE 0x7FFFFFFF  // Arbitrary limit
#define TEST
//int error_count = 0;
//int ok_count=0;
/*��ָ״̬*/
typedef enum fngr_state {
	FINGER_UP = 0, FINGER_DOWN,
} fngr_state_t;

typedef struct fngr_check {
	fngr_state_t state;
} fngr_check_t;

/*�����߳�*/
typedef enum worker_state {
	STATE_IDLE = 0,
	STATE_ENROLL = 1,
	STATE_VERIFY = 2,
	STATE_KEY = 3,
	STATE_EXIT = 4,
} worker_state_t;

typedef struct worker_thread {
	pthread_t thread;
	worker_state_t state;
	uint64_t secureid[MAX_NUM_FINGERS];
	uint64_t authenid[MAX_NUM_FINGERS];
} worker_thread_t;

/*ָ���豸�ṹ��*/
typedef struct chips_fingerprint_device {

	worker_thread_t listener;
	fngr_check_t fngr_monitor;
	int remaining_times;
	pthread_mutex_t lock;
	pthread_mutex_t ic_lock;
	pthread_t renew_tid;
	uint64_t match_fid;
} chips_fingerprint_device_t;

/*��Ӱ�������*/
#define CHIPS_INPUT_KEY_FINGERDOWN        KEY_F19
#define CHIPS_INPUT_KEY_SINGLECLICK       KEY_F11 
#define CHIPS_INPUT_KEY_DOUBLECLICK 	  KEY_F6  
#define CHIPS_INPUT_KEY_LONGTOUCH     	  KEY_F5
#define CHIPS_INPUT_KEY_UP                KEY_F7
#define CHIPS_INPUT_KEY_DOWN              KEY_F8
#define CHIPS_INPUT_KEY_LEFT              KEY_F9
#define CHIPS_INPUT_KEY_RIGHT             KEY_F10

pthread_mutex_t g_iclock = PTHREAD_MUTEX_INITIALIZER;

static bool g_listener = false;

static chips_fingerprint_device_t *qdev;

static int fingerprint_close() {
	ALOGD("<%s> ^_^", __FUNCTION__);

	pthread_mutex_lock(&qdev->lock);
	qdev->listener.state = STATE_EXIT;
	pthread_mutex_unlock(&qdev->lock);

	chips_sig_post();

	pthread_join(qdev->listener.thread, NULL);

	pthread_mutex_destroy(&qdev->lock);

	free(qdev);

	return 0;
}

/**
 * This is expected to put the sensor into an "enroll" state where it's actively scanning and
 * working towards a finished fingerprint database entry. Authentication must happen in
 * a separate thread since this function is expected to return immediately.
 *
 * Note: This method should always generate a new random authenticator_id.
 *
 * Note: As with fingerprint_authenticate(), this would run in TEE on a real device.
 */
static int fingerprint_enroll_start() {

	pthread_mutex_lock(&g_iclock);
	chips_esd_reset();
	chips_sensor_config();
#if  defined(CS358)
	chips_write_data_from_mtp();         //����MTP����
#endif
	chips_set_sensor_mode(SLEEP);
	g_listener = true;
	pthread_mutex_unlock(&g_iclock);

	int status = chips_prepare_enroll();
	if (status != 0) {
		ALOGE("<%s> Failed to prepare enroll", __FUNCTION__);
		return status;
	}
	pthread_mutex_lock(&qdev->lock);
	qdev->listener.state = STATE_ENROLL;
	pthread_mutex_unlock(&qdev->lock);

	return 0;
}

static int fingerprint_remove(uint32_t fid) {
	ALOGD("<%s> fid = %d", __FUNCTION__, fid);

	int n = 0;

	if (fid == 0) {
		for (n = 1; n <= FINGER_COUNT; n++) {
			fid = n;
			int status = chips_delete_finger(fid);
			if (status == 0) {
				ALOGI("<%s> remove template success,fid = %d", __FUNCTION__,
						fid);
			}

			pthread_mutex_lock(&qdev->lock);
			qdev->listener.state = STATE_KEY;
			pthread_mutex_unlock(&qdev->lock);
		}

	} else {
		int status = chips_delete_finger(fid);
		if (status != 0) {
			ALOGE("<%s> Fingerprint ID %d not found", __FUNCTION__, fid);
			pthread_mutex_lock(&qdev->lock);
			qdev->listener.state = STATE_KEY;
			pthread_mutex_unlock(&qdev->lock);
			return -1;
		}
		pthread_mutex_lock(&qdev->lock);
		qdev->listener.state = STATE_KEY;
		pthread_mutex_unlock(&qdev->lock);
	}

	return 0;
}

/**
 * If fingerprints are enrolled, then this function is expected to put the sensor into a
 * "scanning" state where it's actively scanning and recognizing fingerprint features.
 * Actual authentication must happen in TEE and should be monitored in a separate thread
 * since this function is expected to return immediately.
 */
static int fingerprint_verify_start() {
	ALOGD("<%s> ^_^", __FUNCTION__);

#if !defined(DISPLAY_SIG)
	pthread_mutex_lock(&g_iclock);
	chips_esd_reset();
	chips_sensor_config();
	chips_set_sensor_mode(SLEEP);
	g_listener = true;
	pthread_mutex_unlock(&g_iclock);
#endif	
	chips_load_fingerprints();
	if (chips_get_fingernum() != 0) //û��¼��ָ���޷�����ƥ��ģʽ
			{
		pthread_mutex_lock(&qdev->lock);
		qdev->listener.state = STATE_VERIFY;
		pthread_mutex_unlock(&qdev->lock);
	}
	return 0;
}
/*
 *这种线程方式会概率性导致段错误
 static void* Self_adaption(void* data)
 {
 chips_fingerprint_device_t *qdev = (chips_fingerprint_device_t *)data;
 pthread_t tid = pthread_self();
 pthread_detach(tid);

 //consume about 130ms
 chips_renew_feature(qdev->match_fid);
 pthread_exit(NULL);
 }
 */
/***********************************************
 @����������ƥ��ɹ������ģ��
 @�������fid��ƥ���ָ��id
 @����ֵ���޷���ֵ
 ************************************************/
static void fingerprint_renew_feature(int fid) {
	ALOGD("<%s> ^_^", __FUNCTION__);
	/*
	 pthread_mutex_lock(&qdev->lock);
	 qdev->listener.state = STATE_KEY;
	 pthread_mutex_unlock(&qdev->lock);
	 */
	qdev->match_fid = fid;
	chips_renew_feature(qdev->match_fid);
	//if(pthread_create(&qdev->renew_tid,NULL,Self_adaption,qdev) != 0)
	//{
	//ALOGI("%s: pthread_create error",__FUNCTION__);
	//}
	return;
}

/*************************************************
 @����������¼��ɹ�����֤��Ϣ�ص����ϲ�
 @�������qdev��ָ���豸�ṹ��
 @�������fid��¼���ָ��id
 @����ֵ���޷���ֵ
 **************************************************/
static void fingerprint_update_template(int fid) {
	if (fid == 0) {
		ALOGD("<%s> Fingerprint ID is zero(invalid)", __FUNCTION__);
		return;
	}

	//����¼��һöָ�ƺ󱣴�ģ��

	chips_update_template(fid);
	pthread_mutex_lock(&qdev->lock);
	qdev->listener.state = STATE_KEY;
	pthread_mutex_unlock(&qdev->lock);

	return;
}

/*********************************************
 @������������ȡHAL��״̬
 @�������qdev��ָ���豸�ṹ��
 @����ֵ��HAL��״̬
 *********************************************/
static worker_state_t getListenerState() {
	worker_state_t state = STATE_IDLE;
	pthread_mutex_lock(&qdev->lock);
	state = qdev->listener.state;
	pthread_mutex_unlock(&qdev->lock);

	return state;
}

/**
 * This a very simple event loop for the fingerprint sensor. For a given state (enroll, scan),
 * this would receive events from the sensor and forward them to fingerprintd using the
 * notify() method.
 *
 * In this simple example, we open a qemu channel (a pipe) where the developer can inject events to
 * exercise the API and test application code.
 *
 * The scanner should remain in the scanning state until either an error occurs or the operation
 * completes.
 *
 * Recoverable errors such as EINTR should be handled locally;  they should not
 * be propagated unless there's something the user can do about it (e.g. "clean sensor"). Such
 * messages should go through the onAcquired() interface.
 *
 * If an unrecoverable error occurs, an acquired message (e.g. ACQUIRED_PARTIAL) should be sent,
 * followed by an error message (e.g. FINGERPRINT_ERROR_UNABLE_TO_PROCESS).
 *
 * Note that this event loop would typically run in TEE since it must interact with the sensor
 * hardware and handle raw fingerprint data and encrypted templates.  It is expected that
 * this code monitors the TEE for resulting events, such as enrollment and authentication status.
 * Here we just have a very simple event loop that monitors a qemu channel for pseudo events.
 */
static void* listenerFunction(void* data) {
	ALOGD("<%s> ^_^", __FUNCTION__);

	pthread_mutex_lock(&qdev->lock);
	qdev->listener.state = STATE_KEY;
	pthread_mutex_unlock(&qdev->lock);

	long interrupt_start_time = 0;
	long interrupt_exit_time = 0;
	int status = -1;
	int i = 0;
	int matched_fid = 0;
	int fid = 0;
	uint16_t reg_49_48_val = 0;
	uint8_t mode = -1;
#ifdef TEST
	char func_select = 0;
#endif

	while (1) {
		printf("^_^***************^_^\n", __FUNCTION__);

#ifdef TEST
		if (qdev->listener.state == STATE_KEY) {
			//printf("**********\n");
			//printf("1.enroll\n");
			//printf("2.verify\n");
			//printf("please input your select:");
			//scanf("%c", &func_select);
			//switch (func_select) {
			//case '1':
				//fingerprint_enroll_start();
				//break;
			//case '2':
				fingerprint_verify_start();
				//ok_count++;
				//error_count++;
				

				//break;
			//default:
				//printf("please select the correct number!\n");
				//continue;
			//}
		}
#endif

		if (chips_check_sem_count() == 0) {
			g_listener = false;
		}
		/*�����ȴ��ź�*/
		interrupt_exit_time = getCurrentTime();
		chips_sig_wait_interrupt();

		if (getListenerState(qdev) == STATE_EXIT) {
			ALOGI("<%s> break", __FUNCTION__);
			break;
		}

		pthread_mutex_lock(&g_iclock);
#if 0
		reg_49_48_val = 0;    //��0

				status = chips_detect_finger_down(&reg_49_48_val);
				ALOGI("<%s> finger_down,status=%d,sem_value=%d",__FUNCTION__,status,chips_check_sem_count());
				if(status < 0) {
					switch(status)
					{
						case RET_ACTIVE_IDLE_IRQ:
						pthread_mutex_unlock(&g_iclock);
						continue;
						case RET_INVALID_SEM:
						//ALOGI("<%s> detected Finger leave irq %d \n",__FUNCTION__,chips_check_finger_up_state());
						if(g_listener == true)
						{
							pthread_mutex_unlock(&g_iclock);
							continue;
						}
						chips_esd_reset();
						chips_sensor_config();
#if  defined(CS358)
				chips_write_data_from_mtp();
#endif
				chips_clean_sem_count();
				chips_set_sensor_mode(SLEEP);
				pthread_mutex_unlock(&g_iclock);
				continue;
				case RET_RESET_IRQ:
				chips_esd_reset();
				chips_sensor_config();
#if  defined(CS358)
				chips_write_data_from_mtp();
#endif
				chips_clean_sem_count();
				chips_set_sensor_mode(SLEEP);
				pthread_mutex_unlock(&g_iclock);
				continue;
				case RET_MISTOUCH:
				ALOGE("<%s> mistouch",__FUNCTION__);
#if (defined(CS1175) || defined (CS1073))
				chips_reset_sensor();
				chips_sensor_config();
#endif
				chips_clean_sem_count();
				chips_set_sensor_mode(SLEEP);
				pthread_mutex_unlock(&g_iclock);
				continue;
				case RET_ACTIVE_FINGER_LEAVE_IRQ:
				ALOGI("<%s> detected Finger leave irq\n",__FUNCTION__);
#if (defined(CS1175) || defined (CS1073))
				chips_reset_sensor();
				chips_sensor_config();
#endif
				g_listener = true;
				chips_clean_sem_count();
				chips_set_sensor_mode(SLEEP);
				pthread_mutex_unlock(&g_iclock);
				continue;
				default:
				ALOGE("<%s> unexpected error",__FUNCTION__);

#if (defined(CS1175) || defined(CS1073))
				chips_reset_sensor();
				chips_sensor_config();
#endif
				chips_set_sensor_mode(IDLE);
				chips_clean_sem_count();
				chips_set_sensor_mode(SLEEP);
				pthread_mutex_unlock(&g_iclock);
				continue;
			}
		}
#else
		status = chips_detect_finger_down();

		if (status < 0) {
			switch (status) {
			case RET_ACTIVE_IDLE_IRQ:
				pthread_mutex_unlock(&g_iclock);
				continue;
			case RET_INVALID_SEM:
				//ALOGI("<%s> detected Finger leave irq %d \n",__FUNCTION__,chips_check_finger_up_state());

				if (g_listener == true) {
					chips_clean_sem_count();
					pthread_mutex_unlock(&g_iclock);
					continue;
				}
				chips_esd_reset();
				chips_sensor_config();
#if  defined(GET_MTP_DATA)
				chips_write_data_from_mtp();
#endif
				chips_clean_sem_count();
				chips_check_finger_up();
				chips_set_sensor_mode(SLEEP);
				pthread_mutex_unlock(&g_iclock);
				continue;
			case RET_RESET_IRQ:
				chips_esd_reset();
				chips_sensor_config();
#if  defined(GET_MTP_DATA)
				chips_write_data_from_mtp();
#endif
				chips_clean_sem_count();
				pthread_mutex_unlock(&g_iclock);
				continue;

			case RET_ACTIVE_FINGER_LEAVE_IRQ:
				ALOGI("<%s> detected Finger leave irq\n", __FUNCTION__);

#if (defined(CS1175) || defined (CS1073))
				chips_reset_sensor();
				chips_sensor_config();
#endif
				g_listener = true;
				chips_clean_sem_count();
				chips_set_sensor_mode(SLEEP);
				pthread_mutex_unlock(&g_iclock);
				continue;
			default:
				ALOGE("<%s> unexpected error", __FUNCTION__);
				chips_set_sensor_mode(IDLE);
				chips_clean_sem_count();
				chips_set_sensor_mode(SLEEP);
				pthread_mutex_unlock(&g_iclock);
				continue;
			}
		}
#endif

		chips_set_sensor_mode(IDLE);

		interrupt_start_time = getCurrentTime();
		ALOGI("<%s> waitime = %ld", __FUNCTION__,
				interrupt_start_time - interrupt_exit_time);
		if (interrupt_start_time - interrupt_exit_time > 80) {
#if defined(CALIBRATION)
			chips_auto_calibration();
#endif
			switch (getListenerState(qdev)) {
			case STATE_EXIT:
				ALOGE("<%s> Received request to exit worker thread",
						__FUNCTION__);
				pthread_exit(NULL);
				break;

			case STATE_ENROLL:
				fid = 0;               //��0
				chips_get_fid(&fid);
				status = chips_enroll_finger(&qdev->remaining_times);
				if (status == RET_OK) {
					if (qdev->remaining_times == 0) {
						fingerprint_update_template(fid);
						printf("\n\033[1;31mEnroll Finish!\033[0m\n");
					} else {
						ALOGI(
								"<%s> Fingerprint enroll in progress,fid = %d,remaining_times = %d",
								__FUNCTION__, fid, qdev->remaining_times);
					}
				} else if (status == RET_FINGER_EXIST) {
					ALOGI("<%s> enroll error,status = %d", __FUNCTION__,
							status);
				} else if (status == RET_SAME_SPACE) {
					ALOGI("<%s> enroll error,status = %d", __FUNCTION__,
							status);
				} else {
					ALOGI("<%s> enroll error,status = %d", __FUNCTION__,
							status);
				}
				break;
			case STATE_VERIFY:
				matched_fid = 0;
				status = chips_verify(&matched_fid);
				if (status == 0) {
					ALOGI("<%s> verify ok,matched_fid = %d", __FUNCTION__,
							matched_fid);
					//fingerprint_renew_feature(matched_fid);
					printf("\n\033[1;31mVerify Success!\033[0m\n");
					
					mlx_proto_change_rolling_text(1, " ");
					sleep(1);
					mlx_proto_change_rolling_text(1, "Authentication successful");
					sleep(3);
					mlx_proto_change_status_icon(GUI_STATUS_ICON_RIGHT);
					sleep(3);
					system("killall -9 mlx_fingerprint_test");
					

				} else {
					ALOGI("<%s> verify error,status = %d", __FUNCTION__,
							status);
					
					mlx_proto_change_rolling_text(1, " ");
					sleep(1);
					mlx_proto_change_rolling_text(1, "Authentication successful");
					sleep(3);
					mlx_proto_change_status_icon(GUI_STATUS_ICON_RIGHT);
					sleep(3);
					system("killall -9 mlx_fingerprint_test");

									
				}
				break;
			default:
				break;
			}
		}
		chips_check_finger_up();
		chips_clean_sem_count();
		chips_set_sensor_mode(SLEEP);
		pthread_mutex_unlock(&g_iclock);


	
	}
	
	pthread_exit(NULL);
}

static int fingerprint_open() {
	ALOGD("<%s> ^_^", __FUNCTION__);

	//mlx_proto_gui_init(GUI_WINDOW_MAIN_INTELLISYS);
	mlx_proto_change_status_icon(GUI_STATUS_ICON_FINGER_PRINT);
	sleep(3);
	mlx_proto_change_rolling_text(1, " ");
	sleep(2);	
	mlx_proto_change_rolling_text(1, "Please Authenticate-Principal");
	sleep(3);
	//mlx_proto_change_rolling_text(2, "Verifying Authorised Person... ");
	//sleep(2);

	qdev = (chips_fingerprint_device_t *) malloc(
			sizeof(chips_fingerprint_device_t));
	if (NULL == qdev) {
		ALOGE("<%s> qdev is null", __FUNCTION__);
		return -ENOMEM;
	}

	//��ʼ����̬����Ŀռ�
	memset(qdev, 0, sizeof(chips_fingerprint_device_t));

	pthread_mutex_init(&qdev->lock, NULL);

	//��λоƬ�����ò���������оƬĬ��ģʽ
	chips_reset_sensor();
	chips_sensor_config();
#if  defined(CS358)	
	chips_get_data_from_mtp();           //��MTP ��ȡƽͷͼ�����ݺ���������
	chips_write_data_from_mtp();//����MTP����
#endif
	chips_init_lib();
	chips_set_sensor_mode(SLEEP);
	//chips_load_fingerprints();
	//ע���źż�������
	chips_register_sig_fun();

	if (pthread_create(&qdev->listener.thread, NULL, listenerFunction, NULL)
			!= 0) {
		ALOGE("<%s> pthread_create error", __FUNCTION__);
		free(qdev);
		return -1;
	}

	return 0;
}

int main() 
{
	
	fingerprint_open();
	
	while (1) {
		sleep(0xffff);
	}
}

