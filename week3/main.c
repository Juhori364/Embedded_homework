//week3 1 point criteria met

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>

//led config
static const struct gpio_dt_spec red = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

// UART initialization
#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);


// Declarations
#define	STACKSIZE	500
#define	PRIORITY	5
void red_led_task(void *, void *, void*);
void green_led_task(void *, void *, void*);
void yellow_led_task(void *, void *, void*);
static void uart_task(void *, void *, void *);
static void dispatcher_task(void *, void *, void *);
int init_led_red();
int init_led_green();
int init_uart();

//Thread declarations
K_THREAD_DEFINE(red_thread,STACKSIZE,red_led_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(green_thread,STACKSIZE,green_led_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(yellow_thread,STACKSIZE,yellow_led_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(dis_thread,STACKSIZE,dispatcher_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(uart_thread,STACKSIZE,uart_task,NULL,NULL,NULL,PRIORITY,0,0);

//Condition variables
K_MUTEX_DEFINE(red_mutex);
K_CONDVAR_DEFINE(red_signal);
K_MUTEX_DEFINE(green_mutex);
K_CONDVAR_DEFINE(green_signal);
K_MUTEX_DEFINE(yellow_mutex);
K_CONDVAR_DEFINE(yellow_signal);
K_MUTEX_DEFINE(release_mutex);
K_CONDVAR_DEFINE(release_signal);


//Global variables
K_FIFO_DEFINE(dispatcher_fifo);


//fifo dispatcher data type
struct data_t {
	/*************************
	// Add fifo_reserved below
	*************************/
	void *fifo_reserved;
	char msg[20];
};





// Main program
int main(void) {

	init_led_red();
	init_led_green();
	
	int ret = init_uart();
	if (ret != 0) {
		printk("UART initialization failed!\n");
		return ret;
	}

	char rc=0;

	//Test uart by printing input
	while (true) {
		//printk("Hello from main\n");
		k_msleep(1000);

		// Ask UART if data available
		if (uart_poll_in(uart_dev,&rc) == 0) {
			printk("Received: %c\n",rc);
		}

		k_yield();
	}
	
	return 0;
}

//UART init fucntion
int init_uart(void) {
	// UART initialization
	if (!device_is_ready(uart_dev)) {
		return 1;
	} 
	return 0;
}

//led init function
int  init_led_red() {

	// Led pin initialization
	int ret = gpio_pin_configure_dt(&red, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		printk("Error: Led configure failed\n");		
		return ret;
	}
	// set led off
	gpio_pin_set_dt(&red,0);

	printk("Led initialized ok\n");
	
	return 0;
}

int  init_led_green() {

	// Led pin initialization
	int ret = gpio_pin_configure_dt(&green, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		printk("Error: Led configure failed\n");		
		return ret;
	}
	// set led off
	gpio_pin_set_dt(&green,0);

	printk("Led initialized ok\n");
	
	return 0;
}


// Task to handle red led
void red_led_task(void *, void *, void*) {
	
	printk("Red led thread started\n");
	while (true) {
		//wait for signal
		k_condvar_wait(&red_signal, &red_mutex, K_FOREVER);

			gpio_pin_set_dt(&red,1);
			//printk("Red on\n");
			k_sleep(K_SECONDS(1));
			gpio_pin_set_dt(&red,0);
			//printk("Red off\n");
			
		k_condvar_broadcast(&release_signal);
		k_yield();
	}
}

void green_led_task(void *, void *, void*) {
	
	printk("Green led thread started\n");
	while (true) {
		//wait for signal
		k_condvar_wait(&green_signal, &green_mutex, K_FOREVER);
			gpio_pin_set_dt(&green,1);
			//printk("Green on\n");
			k_sleep(K_SECONDS(1));
			gpio_pin_set_dt(&green,0);
			//printk("Green off\n");
		
		k_condvar_broadcast(&release_signal);
		k_yield();
	}
}

void yellow_led_task(void *, void *, void*) {
	
	printk("Yellow led thread started\n");
	while (true) {
		//wait for signal
		k_condvar_wait(&yellow_signal, &yellow_mutex, K_FOREVER);
			gpio_pin_set_dt(&red,1);
			gpio_pin_set_dt(&green,1);
			//printk("Yellow on\n");
			k_sleep(K_SECONDS(1));
			gpio_pin_set_dt(&red,0);
			gpio_pin_set_dt(&green,0);
			//printk("Yellow off\n");
			
		k_condvar_broadcast(&release_signal);
		k_yield();

	}
}




/********************
 * UART task
 */
static void uart_task(void *unused1, void *unused2, void *unused3)
{
	// Received character from UART
	char rc=0;
	// Message from UART
	char uart_msg[20];
	memset(uart_msg,0,20);
	int uart_msg_cnt = 0;

	while (true) {
		// Ask UART if data available
		if (uart_poll_in(uart_dev,&rc) == 0) {
			// printk("Received: %c\n",rc);
			// If character is not newline, add to UART message buffer
			if (rc != '\r') {
				uart_msg[uart_msg_cnt] = rc;
				uart_msg_cnt++;
			// Character is newline, copy dispatcher data and put to FIFO buffer
			} else {
				printk("UART msg: %s\n", uart_msg);
                
				struct data_t *buf = k_malloc(sizeof(struct data_t));
				if (buf == NULL) {
					return;
				}
				// Copy UART message to dispatcher data
				// strncpy(buf->msg, 20, uart_msg); // mitä ihmettä, miksi kaatuu!!
				snprintf(buf->msg, 20, "%s", uart_msg);
				k_fifo_put(&dispatcher_fifo, buf);

				// Clear UART receive buffer
				uart_msg_cnt = 0;
				memset(uart_msg,0,20);

				// Clear UART message buffer
				uart_msg_cnt = 0;
				memset(uart_msg,0,20);
			}
		}
		k_msleep(10);
	}
}

static void dispatcher_task(void *unused1, void *unused2, void *unused3)
{
	while (true) {
		// Receive dispatcher data from uart_task fifo
		struct data_t *rec_item = k_fifo_get(&dispatcher_fifo, K_FOREVER);
		char sequence[20];
		memcpy(sequence,rec_item->msg,20);
		k_free(rec_item);

		printk("Dispatcher: %s\n", sequence);
		int cnt=0;

		//Set on leds according to sequence[]
		while (sequence[cnt] != 0){
			switch (sequence[cnt]){
				case 'r':
				printk("RED \n");
				k_condvar_broadcast(&red_signal);
				break;
				
				case 'y':
				printk("YELLOW \n");
				k_condvar_broadcast(&yellow_signal);
				break;

				case 'g':
				printk("GREEN \n");
				k_condvar_broadcast(&green_signal);
				break;

				default:
				printk("Unknong value in dispatcher fifo \n");
				break;
			}
			//wait for led task to release
			k_condvar_wait(&release_signal, &release_mutex, K_FOREVER);
			cnt++;
		}
	}
}
