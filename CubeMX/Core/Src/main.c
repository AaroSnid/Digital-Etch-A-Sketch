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

#include "ili9488.h"
#include "lis3dhtr.h"
#include <stdint.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

typedef enum {
  SYSTEM_STATE_DRAWING,
  SYSTEM_STATE_ERASING,
  SYSTEM_STATE_DIMMING,
  SYSTEM_STATE_THICKNESS_SELECT,
} system_states;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define ETCH_A_SKETCH_GREY 0xD592

#define MAX_CURSOR_THICKNESS 40   // Capped for visual and performance reasons

// Use classic grey background
#if 0
#define BACKGROUND_COLOR ETCH_A_SKETCH_GREY
#else
#define BACKGROUND_COLOR 0xFFFF
#endif

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
QSPI_HandleTypeDef hqspi;

SPI_HandleTypeDef hspi2;
SPI_HandleTypeDef hspi3;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim16;

/* USER CODE BEGIN PV */

system_states state = SYSTEM_STATE_DRAWING;

lis3dhtr_cfg_t lis3dhtr_cfg;

volatile bool shake_event_flag = 0;
volatile bool confirm_clear_screen_flag = 0;

uint16_t last_x_pos = 1;
uint16_t last_y_pos = 1;
uint16_t previous_selector_cnt = 0;

uint8_t system_line_thickness = 1;
uint8_t previous_system_line_thickness = 1;

volatile bool pb1_interrupt_fired = false;
volatile bool pb2_interrupt_fired = false;


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_QUADSPI_Init(void);
static void MX_SPI2_Init(void);
static void MX_SPI3_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM16_Init(void);
/* USER CODE BEGIN PFP */

static void clear_screen(int colour);
static void fill_cursor_box(uint16_t x, uint16_t y, uint8_t thickness, uint16_t colour);

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
  MX_QUADSPI_Init();
  MX_SPI2_Init();
  MX_SPI3_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_TIM16_Init();
  /* USER CODE BEGIN 2 */

  // Set initial PWM to max duty cycle for full brightness
  TIM16->CCR1 = TIM16->ARR; 
  HAL_TIM_PWM_Start(&htim16, TIM_CHANNEL_1);

  // Start rotary encoder inputs
  HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);
  HAL_TIM_Encoder_Start(&htim1, TIM_CHANNEL_ALL);

  // Configure LCD screen registers
  ILI9488_Init();
  setRotation(3);                             // Set landscape mode. 0, 0 in bottom right
  clear_screen(BACKGROUND_COLOR);    // Set screen to blank colour 
  
  lis3dh_init(&lis3dhtr_cfg, &hspi2, SPI2_CS_GPIO_Port, SPI2_CS_Pin);
  lis3dh_configure_data(&lis3dhtr_cfg, LIS3DH_ODR_100HZ, LIS3DH_2, LIS3DH_MODE_NORMAL, 0x07);
  lis3dh_configure_shake_to_erase(&lis3dhtr_cfg, true);

  // Set default cursor position
  TIM1->CNT = 0;
  TIM2->CNT = 0;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

    if (shake_event_flag == true){

      // Check if device was facing down at time of interrupt
      if (confirm_clear_screen_flag == true){

        clear_screen(BACKGROUND_COLOR);
        confirm_clear_screen_flag = false;
      }

        // Clear interrupt latch from accelerometer
        uint8_t clear_token;
        lis3dh_read_interrupt_source(&lis3dhtr_cfg, LIS3DH_INT_PIN_1, &clear_token);

        shake_event_flag = false;
    }

    // Handle state change outside ISR
    if (pb1_interrupt_fired){
      switch (state) {
        case SYSTEM_STATE_DRAWING:
          // First PB1 press enters dimming mode
          last_x_pos = TIM1->CNT / 4;
          last_y_pos = TIM2->CNT / 4;
          TIM2->CNT = (TIM16->CCR1 * TIM2->ARR) / TIM16->ARR;
          previous_selector_cnt = TIM2->CNT;
          state = SYSTEM_STATE_DIMMING;
          break;

        case SYSTEM_STATE_DIMMING:
          // Second PB1 press enters thickness selection mode
          // last_x/y_pos already hold the original drawing position
          TIM2->CNT = (TIM2->ARR * system_line_thickness) / (MAX_CURSOR_THICKNESS);
          previous_selector_cnt = TIM2->CNT;
          state = SYSTEM_STATE_THICKNESS_SELECT;
          break;

        case SYSTEM_STATE_THICKNESS_SELECT:
          // Third PB1 press returns to normal drawing mode
          TIM1->CNT = last_x_pos * 4;
          TIM2->CNT = last_y_pos * 4;
          state = SYSTEM_STATE_DRAWING;

          // Prevent aritifact from previous state from showing
          fill_cursor_box(last_x_pos, last_y_pos, system_line_thickness, TFT9341_BLACK);
          break;

        default:
          state = SYSTEM_STATE_DRAWING;
          break;
      }
      pb1_interrupt_fired = false;
    }

    if (pb2_interrupt_fired){
        if (state == SYSTEM_STATE_ERASING) {
          state = SYSTEM_STATE_DRAWING;
          // Prevent aritifact from previous state from showing
          fill_cursor_box(last_x_pos, last_y_pos, system_line_thickness, TFT9341_BLACK);
        } else {
          // last_x_pos/last_y_pos already hold the real saved drawing position
          if (state == SYSTEM_STATE_DRAWING) {
            last_x_pos = TIM1->CNT / 4;
            last_y_pos = TIM2->CNT / 4;
          } else {
            // Coming from DIMMING/THICKNESS_SELECT
            TIM1->CNT = last_x_pos * 4;
            TIM2->CNT = last_y_pos * 4;
          }
          state = SYSTEM_STATE_ERASING;
          // Prevent aritifact from previous state from showing
          uint8_t box_thickness = (system_line_thickness > 2) ? (system_line_thickness - 2) : system_line_thickness;
          fill_cursor_box((last_x_pos + 1), (last_y_pos + 1), box_thickness, BACKGROUND_COLOR);
          
        }
        pb2_interrupt_fired = false;
    }

    // Enter code state machine
    switch (state){

      // Drawing cursor tracking encoders
      case (SYSTEM_STATE_DRAWING): {
        // Convert register values to cursor position
        uint16_t x_pos = TIM1->CNT / 4;
        uint16_t y_pos = TIM2->CNT / 4;

        if (x_pos != last_x_pos){
          uint16_t line_length = (x_pos > last_x_pos) ? (x_pos - last_x_pos) : (last_x_pos - x_pos);

          // Check for wrap overflow or underflow
          if (line_length > 440){
            // Normalize to border, recalculate length
            last_x_pos = (last_x_pos > 240) ? 0 : 480;
            line_length = (x_pos > last_x_pos) ? (x_pos - last_x_pos) : (last_x_pos - x_pos);
          }

          // Start point must be on the right
          uint16_t start_point = (x_pos > last_x_pos) ? last_x_pos : x_pos;

          // Draw a centered square the size square the cursor represents
          uint16_t half_thickness = system_line_thickness / 2;
          uint16_t square_x = (start_point >= half_thickness) ? (start_point - half_thickness) : 0;
          uint16_t square_y = (last_y_pos >= half_thickness) ? (last_y_pos - half_thickness) : 0;
          fillRect(square_x, square_y, line_length + system_line_thickness, system_line_thickness, TFT9341_BLACK);
          last_x_pos = x_pos;
        }

        if (y_pos != last_y_pos){
          uint16_t line_length = (y_pos > last_y_pos) ? (y_pos - last_y_pos) : (last_y_pos - y_pos);

          // Check for wrap overflow or underflow
          if (line_length > 300){
            // Normalize to border, recalculate line_length
            last_y_pos = (last_y_pos > 160) ? 0 : 320;
            line_length = (y_pos > last_y_pos) ? (y_pos - last_y_pos) : (last_y_pos - y_pos);
          }

          // Start point must be on the bottom
          uint16_t start_point = (y_pos > last_y_pos) ? last_y_pos : y_pos;

          // Draw a centered square the size square the cursor represents
          uint16_t half_thickness = system_line_thickness / 2;
          uint16_t square_x = (last_x_pos >= half_thickness) ? (last_x_pos - half_thickness) : 0;
          uint16_t square_y = (start_point >= half_thickness) ? (start_point - half_thickness) : 0;
          fillRect(square_x, square_y, system_line_thickness, line_length + system_line_thickness, TFT9341_BLACK);
          last_y_pos = y_pos;
        }
        break;
      }

      case (SYSTEM_STATE_ERASING): {
        // Convert register values to cursor position
        uint16_t erase_x_pos = TIM1->CNT / 4;
        uint16_t erase_y_pos = TIM2->CNT / 4;

        // Since it only regenerates on movement, there should be no flickering (no delay needed)
        if ((erase_x_pos != last_x_pos) || (erase_y_pos != last_y_pos)) {
          uint16_t half_thickness = system_line_thickness / 2;
          uint16_t old_square_x = (last_x_pos >= half_thickness) ? (last_x_pos - half_thickness) : 0;
          uint16_t old_square_y = (last_y_pos >= half_thickness) ? (last_y_pos - half_thickness) : 0;
          fillRect(old_square_x, old_square_y, system_line_thickness, system_line_thickness, BACKGROUND_COLOR);

          last_x_pos = erase_x_pos;
          last_y_pos = erase_y_pos;
          uint16_t new_square_x = (last_x_pos >= half_thickness) ? (last_x_pos - half_thickness) : 0;
          uint16_t new_square_y = (last_y_pos >= half_thickness) ? (last_y_pos - half_thickness) : 0;
          fillRect(new_square_x, new_square_y, system_line_thickness, system_line_thickness, TFT9341_BLACK);

          // Leave only an outline if possible
          if (system_line_thickness > 2){
              fillRect((new_square_x + 1), (new_square_y + 1), (system_line_thickness - 2), (system_line_thickness - 2), BACKGROUND_COLOR);
          }
        }
        break;
    }
      // Screen brightness control
      case (SYSTEM_STATE_DIMMING): {
          uint16_t current_cnt = TIM2->CNT;
          uint16_t delta = (current_cnt > previous_selector_cnt) ? (current_cnt - previous_selector_cnt) : (previous_selector_cnt - current_cnt);

          // A large jump in a single tick means the counter wrapped
          if (delta > (TIM2->ARR / 2)) {
            current_cnt = (current_cnt < previous_selector_cnt) ? TIM2->ARR : 0;
            TIM2->CNT = current_cnt;
          }
          previous_selector_cnt = current_cnt;

          // Convert TIM2 scale to TIM16 scale with integer division
          TIM16->CCR1 = (current_cnt * TIM16->ARR) / (TIM2->ARR);
        break;
      }

      case (SYSTEM_STATE_THICKNESS_SELECT): {
        uint16_t current_cnt = TIM2->CNT;
        uint16_t delta = (current_cnt > previous_selector_cnt) ? (current_cnt - previous_selector_cnt) : (previous_selector_cnt - current_cnt);

        // Same wrap-clamping as the brightness selector above
        if (delta > (TIM2->ARR / 2)) {
          current_cnt = (current_cnt < previous_selector_cnt) ? TIM2->ARR : 0;
          TIM2->CNT = current_cnt;
        }
        previous_selector_cnt = current_cnt;

        // Scale timer counter to size of uint8 with integer division (multiply first else CNT/ARR == 0)
        system_line_thickness = (current_cnt * MAX_CURSOR_THICKNESS) / (TIM2->ARR);
        
        // Minumum line thickness of 1
        if (system_line_thickness == 0){
          system_line_thickness = 1;
        }

        // Clear the area the larger previous thickness so the new one is visible
        if (system_line_thickness < previous_system_line_thickness){
            uint16_t prev_half_thickness = previous_system_line_thickness / 2;
            uint16_t prev_square_x = (last_x_pos >= prev_half_thickness) ? (last_x_pos - prev_half_thickness) : 0;
            uint16_t prev_square_y = (last_y_pos >= prev_half_thickness) ? (last_y_pos - prev_half_thickness) : 0;
            fillRect(prev_square_x, prev_square_y, previous_system_line_thickness, previous_system_line_thickness, BACKGROUND_COLOR);
        }

        // Preview cursor thickness, centered on the true cursor position
        uint16_t half_thickness = system_line_thickness / 2;
        uint16_t square_x = (last_x_pos >= half_thickness) ? (last_x_pos - half_thickness) : 0;
        uint16_t square_y = (last_y_pos >= half_thickness) ? (last_y_pos - half_thickness) : 0;
        fillRect(square_x, square_y, system_line_thickness, system_line_thickness, TFT9341_BLACK);

        previous_system_line_thickness = system_line_thickness;
        break;
      }

      // Unknown state entered, default to drawing 
      default: {
        state = SYSTEM_STATE_DRAWING;
        break;
      }
    }

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
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
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
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
}

/**
  * @brief QUADSPI Initialization Function
  * @param None
  * @retval None
  */
static void MX_QUADSPI_Init(void)
{

  /* USER CODE BEGIN QUADSPI_Init 0 */

  /* USER CODE END QUADSPI_Init 0 */

  /* USER CODE BEGIN QUADSPI_Init 1 */

  /* USER CODE END QUADSPI_Init 1 */
  /* QUADSPI parameter configuration*/
  hqspi.Instance = QUADSPI;
  hqspi.Init.ClockPrescaler = 255;
  hqspi.Init.FifoThreshold = 1;
  hqspi.Init.SampleShifting = QSPI_SAMPLE_SHIFTING_NONE;
  hqspi.Init.FlashSize = 1;
  hqspi.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_1_CYCLE;
  hqspi.Init.ClockMode = QSPI_CLOCK_MODE_0;
  hqspi.Init.FlashID = QSPI_FLASH_ID_1;
  hqspi.Init.DualFlash = QSPI_DUALFLASH_DISABLE;
  if (HAL_QSPI_Init(&hqspi) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN QUADSPI_Init 2 */

  /* USER CODE END QUADSPI_Init 2 */

}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 7;
  hspi2.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi2.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief SPI3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI3_Init(void)
{

  /* USER CODE BEGIN SPI3_Init 0 */

  /* USER CODE END SPI3_Init 0 */

  /* USER CODE BEGIN SPI3_Init 1 */

  /* USER CODE END SPI3_Init 1 */
  /* SPI3 parameter configuration*/
  hspi3.Instance = SPI3;
  hspi3.Init.Mode = SPI_MODE_MASTER;
  hspi3.Init.Direction = SPI_DIRECTION_2LINES;
  hspi3.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi3.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi3.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi3.Init.NSS = SPI_NSS_SOFT;
  hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi3.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi3.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi3.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi3.Init.CRCPolynomial = 7;
  hspi3.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi3.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI3_Init 2 */

  /* USER CODE END SPI3_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_Encoder_InitTypeDef sConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 1920;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  sConfig.EncoderMode = TIM_ENCODERMODE_TI12;
  sConfig.IC1Polarity = TIM_ICPOLARITY_FALLING;
  sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC1Filter = 15;
  sConfig.IC2Polarity = TIM_ICPOLARITY_FALLING;
  sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC2Filter = 15;
  if (HAL_TIM_Encoder_Init(&htim1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_Encoder_InitTypeDef sConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 1280;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  sConfig.EncoderMode = TIM_ENCODERMODE_TI12;
  sConfig.IC1Polarity = TIM_ICPOLARITY_FALLING;
  sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC1Filter = 15;
  sConfig.IC2Polarity = TIM_ICPOLARITY_FALLING;
  sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC2Filter = 15;
  if (HAL_TIM_Encoder_Init(&htim2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief TIM16 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM16_Init(void)
{

  /* USER CODE BEGIN TIM16_Init 0 */

  /* USER CODE END TIM16_Init 0 */

  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM16_Init 1 */

  /* USER CODE END TIM16_Init 1 */
  htim16.Instance = TIM16;
  htim16.Init.Prescaler = 79;
  htim16.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim16.Init.Period = 999;
  htim16.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim16.Init.RepetitionCounter = 0;
  htim16.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim16) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim16) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim16, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim16, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM16_Init 2 */

  /* USER CODE END TIM16_Init 2 */
  HAL_TIM_MspPostInit(&htim16);

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
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, SPI2_CS_Pin|TFT_CS_Pin|TFT_DC_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(TFT_RST_GPIO_Port, TFT_RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : SPI2_CS_Pin TFT_CS_Pin TFT_DC_Pin */
  GPIO_InitStruct.Pin = SPI2_CS_Pin|TFT_CS_Pin|TFT_DC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : EXTI_IMU_INT_1_Pin */
  GPIO_InitStruct.Pin = EXTI_IMU_INT_1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(EXTI_IMU_INT_1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : EXTI_IMU_INT_2_Pin */
  GPIO_InitStruct.Pin = EXTI_IMU_INT_2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(EXTI_IMU_INT_2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : EXTI_PB_1_Pin */
  GPIO_InitStruct.Pin = EXTI_PB_1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(EXTI_PB_1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : TFT_RST_Pin */
  GPIO_InitStruct.Pin = TFT_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(TFT_RST_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : EXTI_PB_2_Pin */
  GPIO_InitStruct.Pin = EXTI_PB_2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(EXTI_PB_2_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
  if (GPIO_Pin == EXTI_PB_1_Pin) {
    pb1_interrupt_fired = true;
  } else if (GPIO_Pin == EXTI_PB_2_Pin) {
    pb2_interrupt_fired = true;
  }
  else if (GPIO_Pin == EXTI_IMU_INT_1_Pin) {
    
    // Read physical INT2 pin state (which mirrors live upside-down status)
    bool is_upside_down = (HAL_GPIO_ReadPin(EXTI_IMU_INT_2_GPIO_Port, EXTI_IMU_INT_2_Pin) == GPIO_PIN_SET);

    if (is_upside_down) {
        confirm_clear_screen_flag = true;
    }
    
    shake_event_flag = true;
  }
}

static void clear_screen(int colour) {
   // Set screen to blank colour without overloading memory
  for (int i = 0; i < 16; i++) {
    fillRect(0, 20 * i, 480, 20, colour);
  }
}

static void fill_cursor_box(uint16_t x, uint16_t y, uint8_t thickness, uint16_t colour){
  
  uint16_t half_thickness = thickness / 2;
  uint16_t square_x = (x >= half_thickness) ? (x - half_thickness) : 0;
  uint16_t square_y = (y >= half_thickness) ? (y - half_thickness) : 0;
  
  fillRect(square_x, square_y, thickness, thickness, colour);
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
