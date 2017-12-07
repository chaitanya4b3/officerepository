#ifndef __CHIPS_SENSOR_H__
#define __CHIPS_SENSOR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
enum mode{
	IDLE = 0,
	NORMAL = 1,
	SLEEP = 2,
	DEEP_SLEEP = 6,	
};

long getCurrentTime(void);
int chips_sig_wait_interrupt(void);
int chips_register_sig_fun(void);
int chips_enable_irq(void);
int chips_disable_irq(void);
int chips_send_key_event(int keycode,int keyvalue);
int chips_get_screen_state(int *screen_state);
void chips_close_fd(void);

static int chips_sfr_read(uint16_t addr,uint8_t *data,uint16_t len);
static int chips_sfr_write(uint16_t addr,uint8_t *data,uint16_t len);
static int chips_sram_read(uint16_t addr,uint8_t *data,uint16_t len);
static int chips_sram_write(uint16_t addr,uint8_t *data,uint16_t len);
static int chips_spi_send_cmd(uint8_t cmd);
int chips_sensor_config(void);

int read_SFR(uint16_t addr,uint8_t *data);
int write_SRAM(uint16_t addr,uint16_t data);
int read_SRAM(uint16_t addr,uint16_t *data);
int write_SFR(uint16_t addr,uint8_t data);
unsigned char read_register(unsigned char addr);
int write_register(unsigned char addr, unsigned char value);
int chips_scan_one_image(uint8_t *buffer, uint16_t len);
int chips_set_sensor_mode(int mode);
#if 0
int chips_detect_finger_down(uint16_t *reg_49_48_val);
#else
int chips_detect_finger_down(void);
#endif
int chips_reset_sensor(void);
int chips_esd_reset(void);
int chips_check_sem_count(void);
void chips_clean_sem_count(void);
void chips_check_finger_up(void);
void chips_check_finger_down(void);
int chips_check_finger_up_state(void);

void chips_get_vcm_from_mtp(void);
void chips_get_fc3e_from_mtp(void);
void chips_get_threshold_value_from_mtp(void);
void chips_write_data_from_mtp(void);
int chips_get_mtp_data(uint8_t *addr,int butlen,uint8_t *buffer);
void chips_sig_post(void);


#ifdef __cplusplus
}
#endif

#endif
