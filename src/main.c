/*
 * Copyright (c) 2019 Manivannan Sadhasivam
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/lorawan/lorawan.h>
#include <errno.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>

#define DEFAULT_RADIO_NODE DT_ALIAS(lora0)
BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(DEFAULT_RADIO_NODE),
	     "No default LoRa radio specified in DT");

#define MAX_DATA_LEN 10

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lorawan_send);

char data[MAX_DATA_LEN] = {'h', 'e', 'l', 'l', 'o', 'w', 'o', 'r', 'l', 'd'};

int main(void)
{
	const struct device *const lora_dev = DEVICE_DT_GET(DEFAULT_RADIO_NODE);
	struct lora_modem_config config;
	int ret;


	config.frequency = 923200000;
	config.bandwidth = BW_125_KHZ;
	config.datarate = SF_7;
	config.preamble_len = 8;
	config.coding_rate = CR_4_5;
	config.iq_inverted = false;
	config.public_network = false;
	config.tx_power = 8;
	config.tx = true;

	ret = lora_config(lora_dev, &config);
	if (ret < 0) {
		LOG_ERR("LoRa config failed");
		return 0;
	}

	struct lorawan_join_config join_cfg;
	uint8_t dev_eui[] = {0xd3, 0xec, 0xe5, 0xd9, 0xae, 0xee, 0x86, 0xa2};
	uint8_t join_eui[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	uint8_t app_key[] = {0xed, 0xd2, 0x25, 0x1f, 0x1c, 0x82, 0xff, 0xdf, 0xd0, 0x9b, 0xfb, 0xea, 0x01, 0xb1, 0x34, 0xf1} ;
	uint16_t dev_nonce = 0;

	join_cfg.mode = LORAWAN_ACT_OTAA;
	join_cfg.dev_eui = dev_eui;
	join_cfg.otaa.join_eui = join_eui;
	join_cfg.otaa.app_key = app_key;
	join_cfg.otaa.nwk_key = app_key;
	join_cfg.otaa.dev_nonce = dev_nonce;

	if (!device_is_ready(lora_dev)) {
		LOG_ERR("%s Device not ready", lora_dev->name);
		return 0;
	}

	printk("Starting LoRaWAN stack.\n");
	ret = lorawan_start();
	if (ret < 0) {
		printk("lorawan_start failed: %d\n\n", ret);
		return(-1);
	}

	do
	{
		printk("Joining network using OTAA, dev nonce: %d \n", dev_nonce);
		ret = lorawan_join(&join_cfg);
		if (ret < 0) {
			
			if ((ret =-ETIMEDOUT)) {
				printf("Timed-out waiting for response.\n");
			} else {
				printk("Join failed (%d)\n", ret);
			}
		} else {
			printk("Join successful.\n");
		}
		dev_nonce ++;
		join_cfg.otaa.dev_nonce = dev_nonce;
		if (ret < 0) {
			// If failed, wait before re-trying.
			k_sleep(K_MSEC(5000));
		}
	} while (ret != 0);
	

	while (1) {
		ret = lorawan_send(2, (uint8_t*)&data, sizeof(data), LORAWAN_MSG_CONFIRMED);
		if (ret < 0) {
			LOG_ERR("LoRa send failed");
			// return 0;
		}
		else{
			LOG_INF("Data sent!");
		}

		/* Send data at 1s interval */
		k_sleep(K_MSEC(5000));
	}
	return 0;
}
