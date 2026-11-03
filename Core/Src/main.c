/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim3;

/* USER CODE BEGIN PV */
// 定義系統運行狀態
typedef enum {
    CURTAIN_IDLE = 0,       // 停止/待機
    CURTAIN_OPENING,        // 開啟中 (漸亮)
    CURTAIN_CLOSING,        // 關閉中 (漸暗)
    CURTAIN_PAUSED          // 誤觸暫停
} CurtainState_t;

volatile CurtainState_t current_state = CURTAIN_IDLE;
volatile CurtainState_t last_state = CURTAIN_IDLE; // 記錄暫停前的方向

// 亮度與計時變數
uint32_t pwm_val = 0;              // 黃燈 PWM 比較值 (0 ~ 1000)
uint32_t step_counter = 0;         // 15秒漸變計數器
uint32_t hint_counter = 0;         // 提示音/綠燈閃爍計數器 (0.5秒亮/滅)
uint8_t hint_toggle = 0;           // 提示切換旗標
uint8_t buzzer_double_beep = 0;    // 誤觸時的兩聲逼逼計數

// 去彈跳變數 (單位: ms)
uint32_t last_btn1_tick = 0;
uint32_t last_btn2_tick = 0;
const uint32_t DEBOUNCE_DELAY = 200;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM3_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
  // 啟動 TIM3 Channel 4 PWM 輸出 (對應 PC9 黃燈)
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, 0); // 初始亮度為 0
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      // 每個迴圈固定延遲 15 毫秒 (15秒總共 1000 個迴圈，剛好對應 1000 步 PWM)
      HAL_Delay(15);

      // ---------------------------------------------------------
      // 1. 窗簾開啟中 (漸亮 15 秒)
      // ---------------------------------------------------------
      if (current_state == CURTAIN_OPENING)
      {
          if (pwm_val < 1000)
          {
              pwm_val++; // 每 15ms 增加 1，1000步剛好 15 秒
              __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, pwm_val);
          }
          else
          {
              // 已經達到最亮 (1000)，停止並回到待機狀態
              current_state = CURTAIN_IDLE;
              HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET);  // 綠燈熄滅
              HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET); // 蜂鳴器熄滅
              continue;
          }

          // 綠燈與蜂鳴器提示：亮 0.5秒，停 0.5秒 (總週期 1秒 = 1000ms / 15ms ≒ 66個迴圈)
          hint_counter++;
          if (hint_counter >= 33) // 約 0.5 秒
          {
              hint_counter = 0;
              hint_toggle = !hint_toggle;
              if (hint_toggle) {
                  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET);   // 綠燈亮
                  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_SET);  // 蜂鳴器響
              } else {
                  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET); // 綠燈滅
                  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET); // 蜂鳴器停
              }
          }
      }
      // ---------------------------------------------------------
      // 2. 窗簾關閉中 (漸暗 15 秒)
      // ---------------------------------------------------------
      else if (current_state == CURTAIN_CLOSING)
      {
          if (pwm_val > 0)
          {
              pwm_val--; // 每 15ms 減少 1，1000步剛好 15 秒
              __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, pwm_val);
          }
          else
          {
              // 已經達到最暗 (0)，停止並回到待機狀態
              current_state = CURTAIN_IDLE;
              HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET);  // 綠燈熄滅
              HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET); // 蜂鳴器熄滅
              continue;
          }

          // 綠燈與蜂鳴器提示：亮 0.5秒，停 0.5秒
          hint_counter++;
          if (hint_counter >= 33)
          {
              hint_counter = 0;
              hint_toggle = !hint_toggle;
              if (hint_toggle) {
                  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET);   // 綠燈亮
                  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_SET);  // 蜂鳴器響
              } else {
                  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET); // 綠燈滅
                  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET); // 蜂鳴器停
              }
          }
      }
      // ---------------------------------------------------------
      // 3. 誤觸暫停狀態
      // ---------------------------------------------------------
      else if (current_state == CURTAIN_PAUSED)
      {
          // 黃燈停在當前亮度 (維持 pwm_val 不變)
          // 紅燈長亮
          HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_SET);
          // 綠燈熄滅
          HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET);

          // 處理誤觸時的兩聲較快逼逼提示
          if (buzzer_double_beep > 0)
          {
              static uint32_t beep_timer = 0;
              beep_timer++;
              if (beep_timer < 10) {
                  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_SET);
              } else if (beep_timer < 20) {
                  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET);
              } else if (beep_timer < 30) {
                  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_SET);
              } else {
                  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET);
                  buzzer_double_beep = 0; // 兩聲結束
                  beep_timer = 0;
              }
          }
      }
      // ---------------------------------------------------------
      // 4. 待機/停止狀態
      // ---------------------------------------------------------
      else
      {
          // 確保所有提示燈號與蜂鳴器關閉
          HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_RESET);  // 紅燈滅
          HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET);  // 綠燈滅
          HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET); // 蜂鳴器滅
          hint_counter = 0;
      }
  }
  /* USER CODE END WHILE */
    /* USER CODE BEGIN 3 */

  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 79;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 999;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

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
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6|GPIO_PIN_8, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PC6 PC8 */
  GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PA12 */
  GPIO_InitStruct.Pin = GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/**
 * @brief 外部中斷回呼函數，處理按鈕 1 與按鈕 2 的觸發事件
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    uint32_t current_tick = HAL_GetTick();

    // ---------------------------------------------------------
    // 按鈕 1 處理 (PC13 - 窗簾開/關切換、繼續動作)
    // ---------------------------------------------------------
    if (GPIO_Pin == GPIO_PIN_13)
    {
        // 軟體去彈跳機制
        if ((current_tick - last_btn1_tick) > DEBOUNCE_DELAY)
        {
            last_btn1_tick = current_tick;

            // 如果目前是暫停狀態，按下按鈕1則恢復原本的方向
            if (current_state == CURTAIN_PAUSED)
            {
                HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_RESET); // 關閉紅燈
                current_state = last_state; // 恢復為暫停前的狀態 (OPENING 或 CLOSING)
            }
            // 如果目前正在運行中，按下按鈕1則視為無效或切換 (依照需求：最亮時再按變成關閉)
            else if (current_state == CURTAIN_IDLE)
            {
                // 關閉紅燈確保狀態乾淨
                HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_RESET);

                if (pwm_val == 0 || pwm_val < 1000)
                {
                    // 點選按鈕1：若未滿，開始開啟 (漸亮)
                    current_state = CURTAIN_OPENING;
                    last_state = CURTAIN_OPENING;
                }
                else if (pwm_val == 1000)
                {
                    // 已經在最亮時再按：變成關閉窗簾 (漸暗)
                    current_state = CURTAIN_CLOSING;
                    last_state = CURTAIN_CLOSING;
                }
            }
            // 執行中的過程中若再按按鈕1（例如開到一半想直接關閉，可視需求調整，此處依題目保留標準邏輯）
            else if (current_state == CURTAIN_OPENING)
            {
                current_state = CURTAIN_CLOSING;
                last_state = CURTAIN_CLOSING;
            }
            else if (current_state == CURTAIN_CLOSING)
            {
                current_state = CURTAIN_OPENING;
                last_state = CURTAIN_OPENING;
            }
        }
    }

    // ---------------------------------------------------------
    // 按鈕 2 處理 (PA12 - 模擬手部誤觸，暫停窗簾)
    // ---------------------------------------------------------
    else if (GPIO_Pin == GPIO_PIN_12)
    {
        // 軟體去彈跳機制
        if ((current_tick - last_btn2_tick) > DEBOUNCE_DELAY)
        {
            last_btn2_tick = current_tick;

            // 只有在窗簾正在動作時（開啟中或關閉中），按下誤觸按鈕才有效
            if (current_state == CURTAIN_OPENING || current_state == CURTAIN_CLOSING)
            {
                last_state = current_state;       // 記錄當前運作方向
                current_state = CURTAIN_PAUSED;   // 進入暫停狀態

                // 觸發兩聲較快逼逼的旗標
                buzzer_double_beep = 1;
            }
        }
    }
}
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
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
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
