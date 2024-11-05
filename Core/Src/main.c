/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32l0xx_hal.h"
#include <string.h>
#include <stdlib.h> // For rand function
#include <stdio.h>

UART_HandleTypeDef huart2;

uint8_t txdata_AT[3] = "AT";
uint8_t txdata_QHTTP[17] = "AT+QHTTPURL=63,80";
uint8_t rxdata[32];  // Buffer to store received
//uint8_t rxdata2[7];
uint8_t txdata_GET[14] = "AT+QHTTPGET=80";
uint8_t txdata_URL[64];


typedef enum {
    STATE_SEND_AT,
    STATE_WAIT_FOR_OK_AT,
    STATE_SEND_QHTTPURL,
    STATE_WAIT_FOR_CONNECT,
    STATE_SEND_URL,
    STATE_WAIT_FOR_OK_URL,
    STATE_SEND_QHTTPGET,
    STATE_WAIT_FOR_QHTTPGET_RESPONSE,
    STATE_COMPLETE
} ModemState;

ModemState state = STATE_SEND_AT;



void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);


void generate_random_sensor_values(int *sensor1, int *sensor2) {
    *sensor1 = (rand() % 41) - 10;  // Random value between -10 and 30
    *sensor2 = (rand() % 41) - 10;  // Random value between -10 and 30
}

void update_txdata_URL(int sensor1, int sensor2) {
    // Format URL with sensor values
    snprintf((char *)txdata_URL, sizeof(txdata_URL), 
             "https://modem-test.vercel.app/sensor-data?sensor1=%02d&sensor2=%02d", 
             sensor1, sensor2);
		
		   // Sikrer at den sidste byte er en null-terminator for at undgå støj
    txdata_URL[sizeof(txdata_URL) - 1] = '\0';
		
}

void reset_rxdata() {
    memset(rxdata, 0, sizeof(rxdata));  // Nulstil modtagebufferen for hver ny modtagelse
}



int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_UART_Init();
	
	

	 // int txdata_QHTTP_sent = 0;
	  //int txdata_GET_sent = 0;
    //int txdata_AT_sent = 0;
    //int txdata_URL_sent = 0;
    int sensor1 = 0, sensor2 = 0;

 

  while (1) {
        switch (state) {
            case STATE_SEND_AT:
                reset_rxdata();
                HAL_UART_Transmit(&huart2, txdata_AT, strlen((char *)txdata_AT), HAL_MAX_DELAY);
                state = STATE_WAIT_FOR_OK_AT;
                break;

            case STATE_WAIT_FOR_OK_AT:
                if (HAL_UART_Receive(&huart2, rxdata, 2, 5000) == HAL_OK && 
                    strncmp((char *)rxdata, "OK", 2) == 0) {
                    reset_rxdata();
                    state = STATE_SEND_QHTTPURL;
											
											
                }
                break;

            case STATE_SEND_QHTTPURL:
                HAL_UART_Transmit(&huart2, txdata_QHTTP, sizeof(txdata_QHTTP), HAL_MAX_DELAY);
                state = STATE_WAIT_FOR_CONNECT;
                break;

            case STATE_WAIT_FOR_CONNECT:
                if (HAL_UART_Receive(&huart2, rxdata, 7, 5000) == HAL_OK ||
                    strstr((char *)rxdata, "CONNECT") != NULL) {
                    reset_rxdata();
                    generate_random_sensor_values(&sensor1, &sensor2);
                    update_txdata_URL(sensor1, sensor2);
                    state = STATE_SEND_URL;
                }
                break;

            case STATE_SEND_URL:
                HAL_UART_Transmit(&huart2, txdata_URL, strlen((char *)txdata_URL), HAL_MAX_DELAY);
                state = STATE_WAIT_FOR_OK_URL;
                break;

            case STATE_WAIT_FOR_OK_URL:
                if (HAL_UART_Receive(&huart2, rxdata, 2, 5000) == HAL_OK ||
                    strncmp((char *)rxdata, "OK", 2) == 0) {
                    reset_rxdata();
                    state = STATE_SEND_QHTTPGET;
                }
                break;

            case STATE_SEND_QHTTPGET:
                HAL_UART_Transmit(&huart2, txdata_GET, sizeof(txdata_GET), HAL_MAX_DELAY);
                state = STATE_WAIT_FOR_QHTTPGET_RESPONSE;
                break;

            case STATE_WAIT_FOR_QHTTPGET_RESPONSE:
                if (HAL_UART_Receive(&huart2, rxdata, sizeof(rxdata), 10000) == HAL_OK ||
                    strstr((char *)rxdata, "+QHTTPGET: 0,200,25") != NULL) {
                    reset_rxdata();
                    state = STATE_COMPLETE;
                }
                break;

            case STATE_COMPLETE:
                HAL_Delay(5000);  // Vent et øjeblik før næste cyklus
                state = STATE_SEND_AT;  // Start en ny cyklus
                break;
        }
    }
}




/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)

{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_5;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
       HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        HAL_Delay(500); // Blink LED to indicate an error
    }
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */






	/*
	while (1) {


          	//if (txdata_AT_sent == 0) {
            if (txdata_AT_sent == 0 || (strstr((char *)rxdata, "+QHTTPGET: 0,200,25") != NULL)) {
							
						memset(rxdata, 0, sizeof(rxdata));  // Ryd modtagebufferen
	          HAL_Delay(1000);
            HAL_UART_Transmit(&huart2, txdata_AT, strlen((char *)txdata_AT), HAL_MAX_DELAY);
            txdata_AT_sent = 1;
						txdata_URL_sent = 0;
        }

           //if (HAL_UART_Receive(&huart2, rxdata, sizeof(rxdata), HAL_MAX_DELAY) == HAL_OK) {
             // HAL_Delay(1000);
              //HAL_UART_Transmit(&huart2, txdata_QHTTP, sizeof(txdata_QHTTP), HAL_MAX_DELAY);
				   
				    // Nulstil buffer og modtag data   
				       //memset(rxdata, 0, sizeof(rxdata));
				
               if (HAL_UART_Receive(&huart2, rxdata, 2, HAL_MAX_DELAY) == HAL_OK) {  // Modtag præcis 2 bytes
                // Tjek om modtagne data præcist matcher "OK"
                if (rxdata[0] == 'O' && rxdata[1] == 'K') {
                 HAL_Delay(1000);
                 HAL_UART_Transmit(&huart2, txdata_QHTTP, sizeof(txdata_QHTTP), HAL_MAX_DELAY);
				  
				    
				
				
                 if (txdata_URL_sent == 0) {
							
                  generate_random_sensor_values(&sensor1, &sensor2);  // Generate new random values
                  update_txdata_URL(sensor1, sensor2);                // Update URL with new values
                
							
   							  HAL_Delay(3000);
                  HAL_UART_Transmit(&huart2, txdata_URL, strlen((char *)txdata_URL), HAL_MAX_DELAY);
                  txdata_URL_sent = 1;
            }

                HAL_Delay(3000);
                HAL_UART_Transmit(&huart2, txdata_GET, sizeof(txdata_GET), HAL_MAX_DELAY);

                //  txdata_URL_sent = 0;  // Reset to allow new values on the next loop
            
						    
						      
						     HAL_Delay(3000);     // Wait for a minute before the next update
					       txdata_AT_sent = 0;
						     txdata_URL_sent =0;
					}   
        }
    }
}
*/

   
	 /*
    while (1) {
        // Debug besked for ny cyklusstart
      //  HAL_UART_Transmit(&huart2, (uint8_t *)"Starting new cycle\r\n", 20, HAL_MAX_DELAY);

        // Send "AT" kommandoen, eller hvis "+QHTTPGET: 0,200,25" er modtaget, start ny cyklus
        if (txdata_AT_sent == 0 || (strncmp((char *)rxdata, "+QHTTPGET: 0,200,25", 16) == 0)) {
            reset_rxdata();
            HAL_Delay(1000);

            // Send "AT" og sæt flag
            HAL_UART_Transmit(&huart2, txdata_AT, strlen((char *)txdata_AT), HAL_MAX_DELAY);
            txdata_AT_sent = 1;
            txdata_URL_sent = 0;
        } 

        // Modtag data og kontroller, om vi har modtaget "OK"
        if (HAL_UART_Receive(&huart2, rxdata, 2, 5000) == HAL_OK) {
            if (strncmp((char *)rxdata, "OK", 2) == 0) {
                HAL_Delay(1000);

                // Debug: bekræft modtagelse af "OK"
                //HAL_UART_Transmit(&huart2, (uint8_t *)"Received OK\r\n", 13, HAL_MAX_DELAY);

                // Send "AT+QHTTPURL=63,80"
                HAL_UART_Transmit(&huart2, txdata_QHTTP, sizeof(txdata_QHTTP), HAL_MAX_DELAY);

                // Generer og send URL én gang pr. cyklus
                if (txdata_URL_sent == 0) {
                    generate_random_sensor_values(&sensor1, &sensor2);
                    update_txdata_URL(sensor1, sensor2);

                    HAL_Delay(3000);
                    HAL_UART_Transmit(&huart2, txdata_URL, strlen((char *)txdata_URL), HAL_MAX_DELAY);
                    txdata_URL_sent = 1;

                    // Debug: bekræft afsendelse af URL
                    //HAL_UART_Transmit(&huart2, (uint8_t *)"URL sent\r\n", 10, HAL_MAX_DELAY);
                }

                // Send "AT+QHTTPGET=80"
                HAL_Delay(3000);
                HAL_UART_Transmit(&huart2, txdata_GET, sizeof(txdata_GET), HAL_MAX_DELAY);

                // Vent på "+QHTTPGET: 0,200,25" for at afslutte cyklussen
               // reset_rxdata();
                if (HAL_UART_Receive(&huart2, rxdata, 16, 10000) == HAL_OK) {
                    if (strncmp((char *)rxdata, "OK+QHTTPGET: 0,200,25", 16) == 0) {
                        HAL_Delay(5000);
                        txdata_AT_sent = 0;
                        txdata_URL_sent = 0;

                        // Debug: bekræft afslutning af cyklus
                       // HAL_UART_Transmit(&huart2, (uint8_t *)"Cycle complete\r\n", 16, HAL_MAX_DELAY);
                    }
                } else {
                    // Fejlmeddelelse, hvis "+QHTTPGET: 0,200,25" ikke modtages
                   // HAL_UART_Transmit(&huart2, (uint8_t *)"Error: No +QHTTPGET response\r\n", 30, HAL_MAX_DELAY);
                }
            }
        }
    }
}

*/
