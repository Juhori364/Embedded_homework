// Week2 one point criteria met


#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

//led config
static const struct gpio_dt_spec red = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);


// Task declarations
void task1(void *, void *, void*);
void task2(void *, void *, void*);
#define	STACKSIZE	500
#define	PRIORITY	5
void red_led_task(void *, void *, void*);
void green_led_task(void *, void *, void*);
void yellow_led_task(void *, void *, void*);
K_THREAD_DEFINE(tid1,STACKSIZE,task1,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(tid2,STACKSIZE,task2,NULL,NULL,NULL,PRIORITY,0,0);

//led thread??
K_THREAD_DEFINE(red_thread,STACKSIZE,red_led_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(green_thread,STACKSIZE,green_led_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(yellow_thread,STACKSIZE,yellow_led_task,NULL,NULL,NULL,PRIORITY,0,0);


int led_state = 0;



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
		if(led_state==0){
			gpio_pin_set_dt(&red,1);
			printk("Red on\n");
			k_sleep(K_SECONDS(1));
			gpio_pin_set_dt(&red,0);
			printk("Red off\n");
			led_state=1;
		}
		k_yield();
	}
}

void green_led_task(void *, void *, void*) {
	
	printk("Green led thread started\n");
	while (true) {
		if(led_state==2){
			gpio_pin_set_dt(&green,1);
			printk("Green on\n");
			k_sleep(K_SECONDS(1));
			gpio_pin_set_dt(&green,0);
			printk("Green off\n");
			led_state=0;
		}
		k_yield();
	}
}

void yellow_led_task(void *, void *, void*) {
	
	printk("Yellow led thread started\n");
	while (true) {
		if(led_state==1){
			gpio_pin_set_dt(&red,1);
			gpio_pin_set_dt(&green,1);
			printk("Yellow on\n");
			k_sleep(K_SECONDS(1));
			gpio_pin_set_dt(&red,0);
			gpio_pin_set_dt(&green,0);
			printk("Yellow off\n");
			led_state=2;
		}
		k_yield();

	}
}


// Task1 function
void task1(void *, void *, void*) {

	while (true) {
		printk("Hello from task 1\n");
		k_msleep(3000);
		// k_yield();
	}
}



// Task2 function
void task2(void *, void *, void*) {

	while (true) {
		printk("Hello from task 2\n");
		k_msleep(5000);
		// k_yield();
		printk("%d", led_state);
	}
}

// Main program
int main(void) {

	init_led_red();
	init_led_green();

	while (true) {
		printk("Hello from main\n");
		k_msleep(1000);
		// k_yield();
	}
	
	return 0;
}
