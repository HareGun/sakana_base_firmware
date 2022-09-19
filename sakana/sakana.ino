/*
  Streaming data from Bluetooth to internal DAC of ESP32
  
  Copyright (C) 2020 Phil Schatzmann
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "BluetoothA2DPSink.h"
#include <Wire.h>
#include "WavePlayer.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/i2s.h"

#define TAG   "sakana"

#define _DEV_TYPE_LYCORIS_
// #define _DEV_TYPE_JIGE_


BluetoothA2DPSink a2dp_sink;

#define AMP_EN_PIN 25
#define AMP_BCLK_PIN 27
#define AMP_LRCLK_PIN 14
#define AMP_DIN_PIN 26

#define LM_OUT_PIN 17
#define RT_IN_PIN 22

extern const unsigned char ngm_48K_wav[];
extern const unsigned char jntm_48K_wav[];

extern const unsigned char sakana_48K_wav[];
extern const unsigned char chinanago_48K_wav[];

void setup() {
  static const i2s_config_t i2s_config = {
      .mode = (i2s_mode_t) (I2S_MODE_MASTER | I2S_MODE_TX),
      .sample_rate = 48000, // corrected by info from bluetooth
      .bits_per_sample = (i2s_bits_per_sample_t) 16, /* the DAC module will only take the 8bits from MSB */
      .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
      .communication_format = I2S_COMM_FORMAT_I2S_MSB,
      .intr_alloc_flags = 0, // default interrupt priority
      .dma_buf_count = 8,
      .dma_buf_len = AMP_DIN_PIN,
      .use_apll = false
  };
  i2s_pin_config_t i2s_pin_config = 
  {
    .bck_io_num = AMP_BCLK_PIN,
    .ws_io_num = AMP_LRCLK_PIN,
    .data_out_num = AMP_DIN_PIN,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  pinMode(AMP_EN_PIN, OUTPUT);
  pinMode(LM_OUT_PIN, INPUT);
  pinMode(RT_IN_PIN, INPUT);
  digitalWrite(AMP_EN_PIN, HIGH);
  delay(10);
  Serial.begin(115200);

  a2dp_sink.set_pin_config(i2s_pin_config);
  a2dp_sink.set_i2s_config(i2s_config);  
  a2dp_sink.start("Sakana");

  WavePlayer_init();
  WavePlayer_setVolumeScale(0.1);
  delay(500);

  Serial.println("sakana start");

#if defined _DEV_TYPE_LYCORIS_
  if(digitalRead(RT_IN_PIN))
  {
    WavePlayer_reqPlay((WAV_Typedef*)sakana_48K_wav);
  }
  else
  {
    WavePlayer_reqPlay((WAV_Typedef*)chinanago_48K_wav);
  }
#elif defined _DEV_TYPE_JIGE_
  if(digitalRead(RT_IN_PIN))
  {
    WavePlayer_reqPlay((WAV_Typedef*)ngm_48K_wav);
  }
  else
  {
    WavePlayer_reqPlay((WAV_Typedef*)jntm_48K_wav);
  }
#else
#endif


}

uint16_t in_sum = 0;
uint16_t counter = 0;
uint16_t cmpTime = 500;
uint16_t cmpVal = 50;
uint16_t startFlag = 0;
uint16_t lastLMOut = 0;

void loop() {

  if(!a2dp_sink.is_connected())
  {
    if(startFlag)
    {
      in_sum += digitalRead(LM_OUT_PIN);
      counter++;
      if(counter > cmpTime)
      {
        if(in_sum < (cmpTime - cmpVal))
        {
          #if defined _DEV_TYPE_LYCORIS_
            if(digitalRead(RT_IN_PIN))
            {
              WavePlayer_reqPlay((WAV_Typedef*)sakana_48K_wav);
            }
            else
            {
              WavePlayer_reqPlay((WAV_Typedef*)chinanago_48K_wav);
            }
          #elif defined _DEV_TYPE_JIGE_
            if(digitalRead(RT_IN_PIN))
            {
              WavePlayer_reqPlay((WAV_Typedef*)ngm_48K_wav);
            }
            else
            {
              WavePlayer_reqPlay((WAV_Typedef*)jntm_48K_wav);
            }
          #else
          #endif
        }
        counter = 0;
        in_sum = 0;
        startFlag = 0;
      }
    }
    else
    {
      uint16_t LMOut = digitalRead(LM_OUT_PIN);
      if(LMOut == 0 && lastLMOut == 1)
      {
        startFlag = 1;
      }
      lastLMOut = LMOut;
    }
  }


  delay(1);
}
