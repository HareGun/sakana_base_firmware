


#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/FreeRTOSConfig.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "WavePlayer.h"
#include "driver/i2s.h"
#include "esp_system.h"
#include "esp_log.h"
#include "Arduino.h"

#define TAG   "WavPlayer"

i2s_port_t i2s_port = I2S_NUM_0;
static uint8_t isRunning = 0;
static uint32_t playingIndex = 0;
static uint8_t* pPlayingBuff = NULL;
static uint32_t playSize = 0;
static float volumeScale = 0.1;
static const uint16_t sendPerSize = 1024;
static int16_t dataBuff[sendPerSize];
xSemaphoreHandle xPlayReqSemaphore = NULL;
xSemaphoreHandle xPlayDoneSemaphore = NULL;
TaskHandle_t xHandleTaskWavePlayer = NULL;


static void vTaskWavePlayer(void *pvParameters);

void WavePlayer_init()
{
  xPlayReqSemaphore = xSemaphoreCreateBinary();
  xPlayDoneSemaphore = xSemaphoreCreateBinary();
  xTaskCreate(vTaskWavePlayer, "TaskWavePlayer", 2048, NULL, 0, &xHandleTaskWavePlayer);
}

void WavePlayer_reqPlay(WAV_Typedef* wav)
{
  if(!isRunning)
  {
    playingIndex = 0;
    playSize = wav->sound_size;
    pPlayingBuff = (uint8_t*)(wav + 1);
    isRunning = 1;
    if(xPlayReqSemaphore != NULL)
		{
      Serial.println("WavePlayer_reqPlay");
      if (i2s_set_clk(I2S_NUM_0, 48000, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO)!=ESP_OK){
        ESP_LOGE(TAG, "i2s_set_clk failed");
      }
			xSemaphoreGive( xPlayReqSemaphore );
		}
  }
}

void WavePlayer_waitPlayDone(void)
{
  if(!isRunning)
  {
    xSemaphoreTake( xPlayDoneSemaphore, 0 );
    return;
  }
  else
  {
    if( xSemaphoreTake( xPlayDoneSemaphore, portMAX_DELAY ) == pdTRUE )
    {
      return;
    }
  }
}

void WavePlayer_setVolumeScale(float scale)
{
  volumeScale = scale;
}

static void WavePlayer_volScale(int16_t *in, int16_t *out, uint16_t size, float scale)
{
  for(int i = 0; i < size; i++)
  {
    out[i] = (float)in[i] * scale;
  }
}

static void vTaskWavePlayer(void *pvParameters)
{
  while(1)
  {
    if( xSemaphoreTake( xPlayReqSemaphore, portMAX_DELAY ) == pdTRUE )
    {
      if(playSize != 0)
      {
        
        uint32_t dataInteger = playSize/sendPerSize;
        uint32_t dataRemainder = playSize%sendPerSize;
        Serial.print("play data:");
        Serial.print(dataInteger);
        Serial.print(", ");
        Serial.println(dataRemainder);

        // ESP_LOGI(TAG, "play data:%d, &d", dataInteger, dataRemainder);
        size_t i2s_bytes_written = 0;
        while(isRunning)
        {
          
          if(dataRemainder != 0 && (playingIndex == (dataInteger*sendPerSize)))
          {
            WavePlayer_volScale((int16_t*)(pPlayingBuff + playingIndex), dataBuff, dataRemainder/2, volumeScale);
            if (i2s_write(i2s_port, dataBuff, dataRemainder, &i2s_bytes_written, portMAX_DELAY)!=ESP_OK){
              ESP_LOGE(TAG, "i2s_write has failed");    
            }
            playingIndex+=dataRemainder;
            isRunning = false;
          }
          else if(0 == dataRemainder && (playingIndex == (dataInteger*sendPerSize-sendPerSize)))
          {
            WavePlayer_volScale((int16_t*)(pPlayingBuff + playingIndex), dataBuff, sendPerSize/2, volumeScale);
            if (i2s_write(i2s_port, dataBuff, sendPerSize, &i2s_bytes_written, portMAX_DELAY)!=ESP_OK){
              ESP_LOGE(TAG, "i2s_write has failed");    
            }
            isRunning = false;
            playingIndex+=sendPerSize;
          }
          else
          {
            WavePlayer_volScale((int16_t*)(pPlayingBuff + playingIndex), dataBuff, sendPerSize/2, volumeScale);
            if (i2s_write(i2s_port, dataBuff, sendPerSize, &i2s_bytes_written, portMAX_DELAY)!=ESP_OK){
              ESP_LOGE(TAG, "i2s_write has failed");    
            }
            playingIndex+=sendPerSize;
          }        
        }
      }

      xSemaphoreGive( xPlayDoneSemaphore );
      isRunning = 0;
    }
  }
  
}

