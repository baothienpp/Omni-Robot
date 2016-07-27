
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#define ExportPath "/sys/class/gpio/export"
#define UnexportPath "/sys/class/gpio/unexport"
#define PinPath "/sys/class/gpio/gpio"
#define PWMPath "/sys/class/pwm/pwmchip"

//Define Functions
char * Pin2Path(int, int);
void FreeGPIO(int);
void CreateGPIO(int ,int );
void WriteGPIO(int ,int );
int ReadGPIO(int);
void PWM_Create(int ,int ); 
void PWM_Control(int,int ,int ,int ); // void PWM_Control(int pwmchip,int Period,int DutyCycle,int Active)
void Motor_Control(int,int,int,int);
int Puls_Count(int,int);
float Distance(int,int);
int delay(int us);

const int PWM_Period = 1000; // 1Khz
const int M_L = 1; // Motor Left
const int M_R = 2; // Motor Right
const int M_B = 3; // Motor Back 
const int Pin = 1;

int init = 0;


//Main Program
int main(){
	//Initialize
	if (init == 0){

		FreeGPIO(171);
		CreateGPIO(171,1);
		
		init = 1;
	}

	while(1){

		if(delay(5000000)){

			WriteGPIO(171,1);

		}else{

			WriteGPIO(171,0);
			sleep(1);
		}
	
	}

	return 0;

}

//---------------Pin2Path------------------------------
char *Pin2Path(int input,int subfolder){
	//Subfolder 0 - no subfolder
	//			1 - /direction
	//			2 - /value

	static char buffer[50];
	static char Path_temp[50];

	strcpy(Path_temp,PinPath);
	sprintf(buffer, "%d", input );

	strcat(Path_temp,buffer);

	switch (subfolder){
		case 0: 
			break;
		case 1:
			strcat(Path_temp,"/direction");
			break;
		case 2:
			strcat(Path_temp,"/value");
			break;
	}

	return Path_temp;
}

//---------------CreateGPIO------------------------------
void CreateGPIO(int Pin,int Direction){
	//Direction 0 - INPUT
	//			1 - OUTPUT

	int fd =0;
	int rc = 0,rc1 = 0;
	char buffer[10];
	
	//Open "/sys/class/gpio/export"
	fd = open(ExportPath, O_WRONLY );

	//Write Pin Number to "/sys/class/gpio/export"
	sprintf(buffer,"%d",Pin); // Convert Pin to String
	rc = write(fd,buffer, 10); 

	
	if(rc > 0 ) {
		//Set Direction of Pin 
		//Open "/sys/class/gpio/export/gpiox/direction"
		//Write IN or OUT

		close(fd); // close export file

		fd = open(Pin2Path(Pin,1), O_WRONLY);

		switch (Direction){

			case 0:
				rc1 = write(fd,"in",10);
				break;

			case 1:
				rc1= write(fd,"out",10);
				break;
		}

		if (rc1 > 0){
			close(fd);
		}

	}
	

}
//---------------WriteGPIO------------------------------
void WriteGPIO(int Pin,int value){
	//value 0 - off
	//		1 - on

	int fd =0;
	int rc = 0;
	char buffer[10];
	
	//Open "/sys/class/gpio/gpiox/value"

	fd = open(Pin2Path(Pin,2), O_WRONLY );
	
	if(fd > 0 ) {
		sprintf(buffer,"%d",value);
		rc = write(fd,buffer,10);
	}

	if(rc > 0){
		close(fd);
	}

}

//---------------ReadGPIO------------------------------
int ReadGPIO(int Pin){
	//result 1 - HIGH
	//		 0 - LOW

	int fd =0;
	int rc = 0;
	char buffer[10];
	int result = 0;
	
	//Open "/sys/class/gpio/gpiox/value"

	fd = open(Pin2Path(Pin,2), O_RDONLY);
	
	if(fd > 0 ) {
		rc = read(fd,buffer,10);
		result = atoi(buffer); // convert str to int
	}

	if(rc > 0){
		close(fd);
	}

	if(result == 0){
		result = 1;
	}else{
		result = 0;
	}

	return result;
}

//---------------FreeGPIO------------------------------
void FreeGPIO(int Pin){

	int fd =0;
	int rc = 0;
	char buffer[10];
	
	//Open "/sys/class/gpio/unexport"
	fd = open(UnexportPath, O_WRONLY );

	//Write Pin Number to "/sys/class/gpio/unexport"

	sprintf(buffer,"%d",Pin); // Convert Pin to String
	rc = write(fd,buffer, 10); 

	close(fd);
	
}

//---------------PWM_Control------------------------------
void PWM_Create(int pwmchip,int Active){
	/* Active 0 - Unexport
			  1 - Export */

	int fd = 0, rc = 0;
	char pwmchip_str[10];
	char Path_temp[50];


	sprintf(pwmchip_str,"%d",pwmchip);
	strcpy (Path_temp,PWMPath);
	strcat (Path_temp,pwmchip_str); // /sys/class/pwm/pwmchipX
	

	//echo 0 > /sys/class/pwm/pwmchip3/unexport
	//echo 0 > /sys/class/pwm/pwmchip3/export
	switch (Active){	

		case 0:
			strcat (Path_temp,"/unexport");
			fd = open(Path_temp, O_WRONLY );
			rc = write(fd,"0",1);
			close(fd);
			break;

		case 1:
			strcat (Path_temp,"/export");
			fd = open(Path_temp, O_WRONLY );
			rc = write(fd,"0",1);
			close(fd);
			break;

	}

}

//---------------PWM_Control------------------------------
void PWM_Control(int pwmchip,int Period,int DutyCycle,int Active){
	/* Active 0 - DISABLE
			  1 - ENABLE
	   Period, DutyCycle unit micro s*/

	//echo X ms >  /sys/class/pwm/pwmchip3/pwm0/period
	//echo  X ms >  /sys/class/pwm/pwmchip3/pwm0/duty_cycle
	//echo 1 > /sys/class/pwm/pwmchip3/pwm0/enable 

	int fd_Period = 0, fd_DutyCycle = 0,fd_Enable = 0, rc = 0;
	int Period_ns= 0 , DutyCycle_ns =0 ;
	char pwmchip_str[10];
	char Path_temp[50];
	char Path_Period[50];
	char Path_DutyCycle[50];
	char Path_Enable[50];
	char Period_str[20];
	char DutyCycle_str[20];

	sprintf(pwmchip_str,"%d",pwmchip);
	strcpy (Path_temp,PWMPath);
	strcat (Path_temp,pwmchip_str); // /sys/class/pwm/pwmchipX

	strcpy(Path_Period,Path_temp);
	strcpy(Path_DutyCycle,Path_temp);
	strcpy(Path_Enable,Path_temp);

	strcat (Path_Period,"/pwm0/period");
	strcat (Path_DutyCycle,"/pwm0/duty_cycle");
	strcat (Path_Enable,"/pwm0/enable");


	if (Active == 1){


		fd_Enable = open(Path_Enable, O_WRONLY);
		fd_Period = open(Path_Period, O_WRONLY);
		fd_DutyCycle = open(Path_DutyCycle, O_WRONLY);

		//Enable 
		write(fd_Enable,"1",1);

		//Period
		Period_ns = Period * 1000;
		sprintf(Period_str,"%d",Period_ns);
		write(fd_Period,Period_str,50);

		//Duty Cycle
		DutyCycle_ns = DutyCycle * 1000;
		sprintf(DutyCycle_str,"%d",DutyCycle_ns);
		write(fd_DutyCycle,DutyCycle_str,50);

		close(fd_Enable);
		close(fd_Period);
		close(fd_DutyCycle);


	}else{

		fd_Enable = open(Path_Enable, O_WRONLY);
		write(fd_Enable,"0",1);
		close(fd_Enable);
	}
}
//---------------Motor_Control------------------------------
void Motor_Control(int pwmchip,int speed,int direction,int Active){
	
}

//---------------Puls_Count------------------------------
int Puls_Count(int Pin,int reset){
	/* Reset 0 - No reset
			 1 - Reset variable count */	

	static int bool = 0;
	static int count = 0;

	if(ReadGPIO(Pin) == 1 && bool == 0){
		count ++;
		bool = 1;

	}

	if(ReadGPIO(Pin) == 0 && bool == 1){

		bool = 0;
	}

	if(reset == 1){
		count = 0;
	}

	return count;
}
//---------------Distance------------------------------
float Distance(int Trigger,int Echo){

	clock_t pulse_start;
	float pulse_duration = 0 , distance = 0;

	WriteGPIO(Trigger,0);

	while(delay(100000)){}


	WriteGPIO(Trigger,1);
	while(delay(10)){}
	WriteGPIO(Trigger,0);


	while(ReadGPIO(Echo) == 0){} // Wait for Echo start

	pulse_start = clock();

	while(ReadGPIO(Echo) == 1){} // Wait for Echo end

	pulse_duration = clock()- pulse_start;

	pulse_duration = pulse_duration/CLOCKS_PER_SEC; // convert to Second

	distance = pulse_duration x 17150 ; // in cm

	return distance;
}
//---------------Delay in micro second------------------------------
int delay(int us){
	
	static clock_t start_t;
	static int micro_s = 0, init =0, state = 0;


	if(init == 0){
		start_t = clock(); // set current time
		init = 1;
	}

	micro_s  = (((float)(clock()- start_t))/CLOCKS_PER_SEC)*1000000; // Delta time , convert to us

	if(micro_s >= us){
		init = 0;
		state = 0;

	}else{

		state = 1;
		
	}

	return state;
}