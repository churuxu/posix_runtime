
#include "tiny_posix_stm32.h"
#include "tiny_posix.h"

#ifndef GPIO_SPEED_FREQ_HIGH
#define GPIO_SPEED_FREQ_HIGH GPIO_SPEED_HIGH
#endif

#define UART_ATTR_GET 0
#define UART_ATTR_SET 1

//=============== 全局对象 =================

GPIO_TypeDef* gpio_ports_[] = {
    GPIOA,
    GPIOB,
#ifdef GPIOC    
    GPIOC,
#endif    
#ifdef GPIOD
    GPIOD,
#endif
#ifdef GPIOE
    GPIOE,
#endif
#ifdef GPIOF
    GPIOF,
#endif
#ifdef GPIOG
    GPIOG,
#endif
};

//gpio中断回调函数
static irq_handler gpio_irqs_[16];



extern void SystemClock_Config();
extern void HAL_Config();

static void gpio_clock_init(){
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();   
    __HAL_RCC_GPIOC_CLK_ENABLE();
#ifdef GPIOD
    __HAL_RCC_GPIOD_CLK_ENABLE();
#endif
#ifdef GPIOE
    __HAL_RCC_GPIOE_CLK_ENABLE();
#endif
#ifdef GPIOF
    __HAL_RCC_GPIOF_CLK_ENABLE();
#endif
#ifdef GPIOG
    __HAL_RCC_GPIOG_CLK_ENABLE();
#endif
}


void tiny_posix_init(){    
    HAL_Init();

    gpio_clock_init();
    
    SystemClock_Config();

    HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

    HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

    HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);

    HAL_Config();    
}


int gpio_init(int fd, int mode, int pull){
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin = GPIO_FD_GET_PIN(fd);
    GPIO_InitStruct.Mode = mode;
    GPIO_InitStruct.Pull = pull;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIO_FD_GET_PORT(fd), &GPIO_InitStruct);   
    //gpio_reset(fd);
    return 0;    
}

void gpio_set_irq(int fd, irq_handler func){
    IRQn_Type IRQnb = EXTI0_IRQn;
    uint32_t priority = 0;
    uint16_t pin = GPIO_FD_GET_PIN(fd);
    int index = 0;
    switch(pin) {
        case GPIO_PIN_0:  index = 0;  IRQnb = EXTI0_IRQn;	break;
        case GPIO_PIN_1:  index = 1;  IRQnb = EXTI1_IRQn;	break;
        case GPIO_PIN_2:  index = 2;  IRQnb = EXTI2_IRQn;	break;
        case GPIO_PIN_3:  index = 3;  IRQnb = EXTI3_IRQn;	break;
        case GPIO_PIN_4:  index = 4;  IRQnb = EXTI4_IRQn;	break;
        case GPIO_PIN_5:  index = 5;  IRQnb = EXTI9_5_IRQn; break;
        case GPIO_PIN_6:  index = 6;  IRQnb = EXTI9_5_IRQn;	break;
        case GPIO_PIN_7:  index = 7;  IRQnb = EXTI9_5_IRQn;	break;
        case GPIO_PIN_8:  index = 8;  IRQnb = EXTI9_5_IRQn;	break;
        case GPIO_PIN_9:  index = 9;  IRQnb = EXTI9_5_IRQn; break;
        case GPIO_PIN_10: index = 10; IRQnb = EXTI15_10_IRQn; break;
        case GPIO_PIN_11: index = 11; IRQnb = EXTI15_10_IRQn; break;
        case GPIO_PIN_12: index = 12; IRQnb = EXTI15_10_IRQn; break;
        case GPIO_PIN_13: index = 13; IRQnb = EXTI15_10_IRQn; break;
        case GPIO_PIN_14: index = 14; IRQnb = EXTI15_10_IRQn; break;
        case GPIO_PIN_15: index = 15; IRQnb = EXTI15_10_IRQn; break;
        default:return;
    }
    gpio_irqs_[index] = func;

    HAL_NVIC_SetPriority(IRQnb, priority, 0);
    HAL_NVIC_EnableIRQ(IRQnb);    
}



int gpio_config(int fd, int key, void* value){
    return 0;
}
int gpio_read(int fd, void* buf, int len){
    uint8_t* pbuf = (uint8_t*)buf;
    int val = gpio_status(fd);
    *pbuf = val?1:0;
    return 1;
}
int gpio_write(int fd, const void* buf, int len){
    uint8_t* pbuf = (uint8_t*)buf;
    if(*pbuf){
        gpio_set(fd);
    }else{
        gpio_reset(fd);
    }    
    return 1;
}



//======================== uart ========================

USART_TypeDef* uarts_[] = {
    USART1,
    USART2,
#ifdef USART3   
    USART3,
#endif     
#ifdef UART4
    UART4,
#endif 
#ifdef UART5
    UART5,
#endif     
};


UART_HandleTypeDef uart_handles_[sizeof(uarts_)/sizeof(void*)];



static UART_HandleTypeDef* uart_get_handle(int fd){
    int index = (fd>>8);    
    return &(uart_handles_[index]);
}

static int uart_get_attr(int fd, struct termios* attr){


    return 0;
}
static int uart_set_attr(int fd, const struct termios* attr){
    int baud;
    int stopbits;
    int parity;
    int index = (fd>>8);
    UART_HandleTypeDef* uart = uart_get_handle(fd);   
    
    baud = ((attr->c_cflag & 0xffff) * 100);
    stopbits = (attr->c_cflag & CSTOPB)? UART_STOPBITS_2 : UART_STOPBITS_1;
    parity = (attr->c_cflag & PARENB)?((attr->c_cflag & PARODD)?UART_PARITY_ODD:UART_PARITY_EVEN):UART_PARITY_NONE;
     
    uart->Instance = uarts_[index];
    uart->Init.BaudRate = baud;
    uart->Init.WordLength = UART_WORDLENGTH_8B;
    uart->Init.StopBits = stopbits;
    uart->Init.Parity = parity;
    uart->Init.Mode = UART_MODE_TX_RX;
    uart->Init.HwFlowCtl = UART_HWCONTROL_NONE;
    uart->Init.OverSampling = UART_OVERSAMPLING_16;
    if(HAL_OK != HAL_UART_Init(uart)){
        return -1;
    }
    return 0;
}

int uart_init(int fd, int tx, int rx, int flags){
    struct termios attr;
    int index = (fd>>8);

    //clk enable
    switch(index){
        case 0: __HAL_RCC_USART1_CLK_ENABLE(); break;
        case 1: __HAL_RCC_USART2_CLK_ENABLE(); break;
#ifdef USART3        
        case 2: __HAL_RCC_USART3_CLK_ENABLE(); break;
#endif
#ifdef UART4        
        case 3: __HAL_RCC_UART4_CLK_ENABLE(); break;
#endif
#ifdef UART5 
        case 4: __HAL_RCC_UART5_CLK_ENABLE(); break;
#endif        
        default:return -1;
    }
    
    gpio_init(tx, GPIO_MODE_AF_PP, GPIO_NOPULL);
    gpio_init(rx, GPIO_MODE_INPUT, GPIO_NOPULL);

    attr.c_cflag = flags;
    return uart_set_attr(fd, &attr);    
}

int uart_config(int fd, int key, void* value){
    //struct termios* attr;
    switch(key){
        case UART_ATTR_SET:

        case UART_ATTR_GET:

        default:
            return 0;
    }
    return 0;
}

int uart_read(int fd, void* buf, int len){
    UART_HandleTypeDef* uart = uart_get_handle(fd);
    if(!uart)return -1;
    /*  
    if(HAL_OK == HAL_UART_Receive(uart, (uint8_t*)buf, len, 3000)){
        return len;
    } */      
    return 0;
}
int uart_write(int fd, const void* buf, int len){
    UART_HandleTypeDef* uart = uart_get_handle(fd); 
    if(!uart)return -1; 
    if(HAL_OK == HAL_UART_Transmit(uart, (uint8_t*)buf, len, 3000)){
        return len;
    }
    return -1;
}


int _tp_cfsetispeed(struct termios* attr, speed_t t){
    attr->c_cflag &= 0x0000;
    attr->c_cflag |= t;
    return 0;
}
int _tp_cfsetospeed(struct termios* attr, speed_t t){
    attr->c_cflag &= 0x0000;
    attr->c_cflag |= t;
    return 0;    
}
int _tp_tcgetattr(int fd, struct termios* attr){
    return uart_get_attr(fd, attr);
}
int _tp_tcsetattr(int fd, int opt, const struct termios* attr){
    return uart_set_attr(fd, attr);
}
//=====================================================


//======================== spi ========================

SPI_TypeDef* spis_[] = {
    SPI1,
#ifdef SPI2
    SPI2,
#endif
#ifdef SPI3
    SPI3,
#endif
};


SPI_HandleTypeDef spi_handles_[4];

int spi_init(int fd){
    return 0;
}



//=====================================================


unsigned int _tp_sleep(unsigned int seconds){
    HAL_Delay(seconds * 1000);
    return 0;
}
unsigned int _tp_usleep(unsigned int micro_seconds){
    HAL_Delay(micro_seconds / 1000);
    return 0;
}









#define HANDLE_GPIO_IRQ(pin, index) \
    if(__HAL_GPIO_EXTI_GET_IT(pin) != RESET){\
        __HAL_GPIO_EXTI_CLEAR_IT(pin);\
        if(gpio_irqs_[index])gpio_irqs_[index]();\
    }

//============= 各系统中断 ===============
void SysTick_Handler(){ 
    HAL_IncTick();
    HAL_SYSTICK_IRQHandler(); 
}
void EXTI0_IRQHandler(){
    HANDLE_GPIO_IRQ(GPIO_PIN_0, 0);
}
void EXTI1_IRQHandler(){
    HANDLE_GPIO_IRQ(GPIO_PIN_1, 1);
}
void EXTI2_IRQHandler(){
    HANDLE_GPIO_IRQ(GPIO_PIN_2, 2);
}
void EXTI3_IRQHandler(){
    HANDLE_GPIO_IRQ(GPIO_PIN_3, 3);
}
void EXTI4_IRQHandler(){
    HANDLE_GPIO_IRQ(GPIO_PIN_4, 4);
}
void EXTI9_5_IRQHandler(){
    HANDLE_GPIO_IRQ(GPIO_PIN_5, 5);
    HANDLE_GPIO_IRQ(GPIO_PIN_6, 6);
    HANDLE_GPIO_IRQ(GPIO_PIN_7, 7);
    HANDLE_GPIO_IRQ(GPIO_PIN_8, 8);
    HANDLE_GPIO_IRQ(GPIO_PIN_9, 9);
}
void EXTI15_10_IRQHandler(){
    HANDLE_GPIO_IRQ(GPIO_PIN_10, 10);
    HANDLE_GPIO_IRQ(GPIO_PIN_11, 11);
    HANDLE_GPIO_IRQ(GPIO_PIN_12, 12);
    HANDLE_GPIO_IRQ(GPIO_PIN_13, 13);
    HANDLE_GPIO_IRQ(GPIO_PIN_14, 14);
    HANDLE_GPIO_IRQ(GPIO_PIN_15, 15);
}

