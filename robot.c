
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
#define EncoderPath "/dev/encoder"

//Define Functions
char * Pin2Path(int, int);
void FreeGPIO(int);
void CreateGPIO(int ,int );
void WriteGPIO(int ,int );
int ReadGPIO(int);
void PWM_Create(int ,int ); //void PWM_Create(int pwmchip,int Active)
void PWM_Control(int,int ,int ,int ); // void PWM_Control(int pwmchip,int Period,int DutyCycle,int Active)
void Motor_Control(int,int,int,int,int,int); //void Motor_Control(int pwmchip,int left,int right,int speed,int direction,int Active)
int Puls_Count(int,int);
float Distance(int,int);
int udelay(int us);
void Motor_Brake();
void Go_Straight(int,int);
void Motor_Shutdown(int );
void Rotate(int,int);
float Puls2Distance(int );
float Puls2Degree(int );
int GetEncoder();


const int PulsRotateDef = 55; // 1 Wheel rotation = x puls , 30 cm
const int PWM_Period = 1000; // 1Khz
const int MLeftSpeed = 4; // SD1_CMD pwm4
const int MLeftForward = 160; // CSI0_DAT14
const int MLeftBackward = 161; //CSI0_DAT15
const int MRightSpeed = 3; //SD1_DAT1__PWM3_OUT 
const int MRightForward= 162; // CSI0_DAT16
const int MRightBackward = 163; // CSI0_DAT17
const int MBackSpeed = 2; //  SD1_DAT2__PWM2_OUT
const int MBackForward = 164; // CSI0_DAT18
const int MBackBackward = 165; // CSI0_DAT19

const int Trigger = 158; //CSI0_DAT12
const int Echo = 159; //CSI0_DAT13

const int MLeftEnc = 205; //GPIO_18
const int MRightEnc = 3; //GPIO_3
const int MBackEnc = 204; //GPIO_17


const int Pin = 1; 
int MLeftEnc_R;
int init = 0;
int point = 0;
int abstand = 0;

//Main Program
int main(){

	//Initialize
	if (init == 0){
		//Free resource before initialize
		FreeGPIO(Trigger);
		FreeGPIO(Echo);

		// FreeGPIO(MLeftEnc);
		// FreeGPIO(MRightEnc); // Integrated in Kernel Module !
		// FreeGPIO(MBackEnc);

		PWM_Create(MLeftSpeed,0);
		FreeGPIO(MLeftForward);
		FreeGPIO(MLeftBackward);

		PWM_Create(MRightSpeed,0);
		FreeGPIO(MRightForward);
		FreeGPIO(MRightBackward);

		PWM_Create(MBackSpeed,0);
		FreeGPIO(MBackForward);
		FreeGPIO(MBackBackward);

		//Define Ports
		CreateGPIO(Trigger,1);
		CreateGPIO(Echo,0);

		// CreateGPIO(MLeftEnc,0);
		// CreateGPIO(MRightEnc,0); // Integrated in Kernel Module !
		// CreateGPIO(MBackEnc,0);

		PWM_Create(MLeftSpeed,1);
		CreateGPIO(MLeftForward,1);
		CreateGPIO(MLeftBackward,1);

		PWM_Create(MRightSpeed,1);
		CreateGPIO(MRightForward,1);
		CreateGPIO(MRightBackward,1);

		PWM_Create(MBackSpeed,1);
		CreateGPIO(MBackForward,1);
		CreateGPIO(MBackBackward,1);

		
		if( access("encoder_driver.ko", F_OK ) != -1 ) {

			//Remove Encoder Driver (if existed)
			system("rmmod encoder_driver");
			
			//Install Encoder Driver
			system("insmod encoder_driver.ko");

		} else {

		   printf("Encoder Kernel Module doesn't existed \n");
		   return 0;
		}

		init = 1;
	}

	while(1){
		printf("%d\n",GetEncoder() );
	}
	return 0;

}
//---------------Puls2Distance------------------------------
float Puls2Distance(int Puls){
	float distance;

	distance = (Puls*0.486) - 1.3;

	return distance;
}
//---------------Puls2Distance------------------------------
float Puls2Degree(int Puls){
	float degree;

	degree = (Puls*1.81) - 7.3;

	return degree;
}
//---------------GetEncoder------------------------------
int GetEncoder(){

	static int fd =0;
	int rc = 0;
	char buffer[256];
	int result = 0;
	
	//Open "/dev/encoder"
	if( fd == 0){
		fd = open(EncoderPath,O_RDONLY);
	}


	if(fd > 0 ) {
		rc = read(fd,buffer,256);
		result = atoi(buffer); // convert str to int
		printf("%s \n",buffer );
	}

	if(rc > 0){
		close(fd);
	}

	return result;
}
//---------------Motor_Brake------------------------------
void Motor_Brake(){
			Motor_Control(MLeftSpeed,MLeftForward,MLeftBackward,100,2,1);
			Motor_Control(MRightSpeed,MRightForward,MRightBackward,100,2,1);
			Motor_Control(MBackSpeed,MBackForward,MBackBackward,100,2,1);		
}

//---------------Motor_Shutdown------------------------------
void Motor_Shutdown(int motors){
	//motors 0 - 2 front motors
	//	 	 1 - all

	if (motors == 0){
		Motor_Control(MLeftSpeed,MLeftForward,MLeftBackward,100,1,0);
		Motor_Control(MRightSpeed,MRightForward,MRightBackward,100,1,0);			
	}else{
		Motor_Control(MLeftSpeed,MLeftForward,MLeftBackward,100,1,0);
		Motor_Control(MRightSpeed,MRightForward,MRightBackward,100,1,0);
		Motor_Control(MBackSpeed,MBackForward,MBackBackward,100,1,0);		
	}

}

//---------------Go_Straight------------------------------
void Go_Straight(int direction,int speed){
	//Direction 0 - Forward
	//			1 - Backward
	if (direction == 0){
		Motor_Control(MLeftSpeed,MLeftForward,MLeftBackward,speed,1,1);
		Motor_Control(MRightSpeed,MRightForward,MRightBackward,speed,0,1);
	}else{
		Motor_Control(MLeftSpeed,MLeftForward,MLeftBackward,speed,0,1);
		Motor_Control(MRightSpeed,MRightForward,MRightBackward,speed,1,1);
	}		
}
//---------------Rotate------------------------------
void Rotate(int direction,int speed){
	//Direction 1 - Clockwise
	//			0 - Counterclockwise
	if (direction == 0){
			Motor_Control(MLeftSpeed,MLeftForward,MLeftBackward,speed,1,1);
			Motor_Control(MRightSpeed,MRightForward,MRightBackward,speed,1,1);
			Motor_Control(MBackSpeed,MBackForward,MBackBackward,speed,1,1);
	}else{
			Motor_Control(MLeftSpeed,MLeftForward,MLeftBackward,speed,0,1);
			Motor_Control(MRightSpeed,MRightForward,MRightBackward,speed,0,1);
			Motor_Control(MBackSpeed,MBackForward,MBackBackward,speed,0,1);
	}				
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

	int fd =0 ;
	int rc = 0;
	char buffer[10];
	
	//Open "/sys/class/gpio/gpiox/value"

	fd = open(Pin2Path(Pin,2), O_WRONLY );
	
	if(fd > 0 ) {
		sprintf(buffer,"%d",value);
		rc = write(fd,buffer,strlen(buffer));
	}

	if(rc > 0){
		close(fd);
	}

}

//---------------ReadGPIO------------------------------
int ReadGPIO(int Pin){
	//result 1 - HIGH
	//		 0 - LOW

	
	static int fd =0;
	int rc = 0;
	char buffer[10];
	int result = 0;
	
	//Open "/sys/class/gpio/gpiox/value"
	if( fd == 0){
		fd = open(Pin2Path(Pin,2), O_RDONLY);
	}


	if(fd > 0 ) {
		rc = read(fd,buffer,5);
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
	rc = write(fd,buffer, strlen(buffer)); 

	close(fd);
	
}

//---------------PWM_Control------------------------------
void PWM_Create(int pwmchip,int Active){
	/* Active 0 - Unexport
			  1 - Export */

	int fd = 0, rc = 0;
	char pwmchip_str[10];
	char Path_temp[50];

	pwmchip = pwmchip - 1;
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
	   Period unit micro s
	   DutyCycle in % from 1-100 */

	//echo X ms >  /sys/class/pwm/pwmchip3/pwm0/period
	//echo  X ms >  /sys/class/pwm/pwmchip3/pwm0/duty_cycle
	//echo 1 > /sys/class/pwm/pwmchip3/pwm0/enable 
	 
	int err = 0;  
	int fd_Period = 0, fd_DutyCycle = 0,fd_Enable = 0, rc = 0;
	int Period_ns= 0 , DutyCycle_ns =0 ;
	char pwmchip_str[10];
	char Path_temp[50];
	char Path_Period[50];
	char Path_DutyCycle[50];
	char Path_Enable[50];
	char Period_str[20];
	char DutyCycle_str[20];

	pwmchip = pwmchip - 1;
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
		write(fd_Period,Period_str,strlen(Period_str));


		//Duty Cycle
		DutyCycle_ns = DutyCycle * Period_ns / 100;
		sprintf(DutyCycle_str,"%d",DutyCycle_ns);
		write(fd_DutyCycle,DutyCycle_str,strlen(DutyCycle_str));

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
void Motor_Control(int pwmchip,int left,int right,int speed,int direction,int Active){
	/* Direction 0 - Left
			 	 1 - Right 
			 	 2 - Brake */	

	if (Active == 0){
		PWM_Control(pwmchip,PWM_Period,0,Active);
	}

	if (direction == 0){
		WriteGPIO(left,1);
		WriteGPIO(right,0);
		PWM_Control(pwmchip,PWM_Period,speed,Active);
	}

	if(direction ==1){
		WriteGPIO(right,1);
		WriteGPIO(left,0);
		PWM_Control(pwmchip,PWM_Period,speed,Active);
	}

	if(direction == 2){
		WriteGPIO(right,1);
		WriteGPIO(left,1);
		PWM_Control(pwmchip,PWM_Period,speed,Active);
	}



}

//---------------Puls_Count------------------------------
int Puls_Count(int fd,int reset){
	/* Reset 0 - No reset
			 1 - Reset variable count */	

	static int bool = 0;
	static int count = 0;

	if(reset == 1){
		count = 0;
	}

	if(ReadGPIO(fd) == 1 && bool == 0){
		count++;
		bool = 1;

	}

	if(ReadGPIO(fd) == 0 && bool == 1){

		bool = 0;
	}


	return count;
}
//---------------Distance------------------------------
float Distance(int Trigger,int Echo){

	clock_t pulse_start =0;
	float pulse_duration = 0 , distance = 0;

	WriteGPIO(Trigger,0);

	while(udelay(100000)){}


	WriteGPIO(Trigger,1);
	printf("Trigger On\n" );
	while(udelay(10)){}
	WriteGPIO(Trigger,0);
	printf("Echo On\n");


	while(ReadGPIO(Echo) == 1){} // Wait for Echo start

	pulse_start = clock();

	while(ReadGPIO(Echo) == 0){} // Wait for Echo end


	pulse_duration = clock()- pulse_start;

	pulse_duration = pulse_duration/CLOCKS_PER_SEC; // convert to Second

	distance = pulse_duration * 17150; // in cm


	printf("%0.2f CM \n",distance );
	return distance;
}
//---------------Delay in micro second------------------------------
int udelay(int us){
	
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