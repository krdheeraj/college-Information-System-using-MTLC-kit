#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/input.h>

#include <string.h>
#include <sys/dir.h>
#include <linux/fb.h>
#include <ctype.h>
#include <sys/mman.h>
#include <signal.h>
#include <sys/ioctl.h>

#define  NODE_CLCD	"/dev/fpga_char_lcd"
#define  NODE_SEG       "/dev/7seg_led"

#define  LCD_ON              1
#define  LCD_CLEAR           2
#define  LCD_SET_HOME        3
#define  LCD_CURSOR_BLINK    4
#define  LCD_DATA_WRITE      5
#define  LCD_OFF	     6

#define  SEG_7_LED_ON              1
#define  SEG_7_LED_WRITE           2
#define  SEG_7_LED_CLEAR           3
#define  SEG_7_LED_OFF             4

/*typedef struct {
char *name;
int 
int century;
} CollegeInfo;
*/
typedef struct {
char *name;
int rtuc;
int estd;
} CollegeInfo;
typedef struct {
char *name;
char *warden;
int nob;
} HostelInfo;
typedef struct {
char *bname;
char *aname;
int nos;
} AdmissionInfo;

typedef struct {
char *cname;
char *fname;
int nod;
} FestInfo;

CollegeInfo list_college[] = {
	{"PCE", "28", 2000},
	{"PIET", "72",2008 },
	{"PGI", "104",2010},
	{"PU", "Not Known", 2012},
};

HostelInfo list_hostel[] = {
	{"Gurushikhar", "Mr.Ashok Poornia",7 },
	{"Gayatri", "Mrs. Sudha Jain",6 },
	{"Aravali", "Mr.Randeep Hada",7 },
	{"Himalaya", "Mr. Mukseh Tacker",7 },
};

AdmissionInfo list_admission[] = {
	{"ME", "PCE/PGI", 240},
	{"CIV", "PCE/PIET/PGI", 260},
	{"CS/IT", "PCE/PIET/PGI", 240},
	{"EC/EE", "PCE/PIET/PGI", 180},
};

FestInfo list_fest[] = {
	{"Aarohan", "PCE/PIET/PGI",5},
	{"Aayam","Gurushikhar", 4},
	{"Yuvaam","Gaytri",3 },
	{"Param", "Aravali", 3},
};

int exp_clcd= 0;
int ret_clcd, ctr=0;
int exp_seg= 0;
int ret_seg,res= 0;

/***** GLCD *****/
unsigned short Type;
unsigned long Size; /* file size in bytes */

struct FileHeader {
	unsigned short Reserved1; /* 0 */
	unsigned short Reserved2; /* 0 */
	unsigned long OffBits; /* offset to bitmap */
	unsigned long StructSize; /* size of this struct (40) */
	unsigned long Width; /* bmap width in pixels */
	unsigned long Height; /* bmap height in pixels */
	unsigned short Planes; /* num planes - always 1 */
	unsigned short BitCount; /* bits per pixel */
	unsigned long Compression; /* compression flag */
	unsigned long SizeImage; /* image size in bytes */
	long XPelsPerMeter; /* horz resolution */
	long YPelsPerMeter; /* vert resolution */
	unsigned long ClrUsed; /* 0 -> color table size */
	unsigned long ClrImportant; /* important color count */
};

struct RGBQUAD {
	unsigned char rgbBlue;
	unsigned char rgbGreen;
	unsigned char rgbRed;
	unsigned char rgbReserved;
};

extern  int alphasort();

struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;

char *fbp = 0;
int fbfd = 0;
long int screensize;
char *fname[100];
int pid;

void ex_program(int sig)
{
	munmap(fbp, screensize);
	close(fbfd);
	system("pkill -9 -f X");
	exit(0);
}

/***** SHOW_BMP*****/
int show_bmp(char *fname)
{
	FILE *fp;
	unsigned long int location = 0, BytesPerLine = 0;
	unsigned long pixel, p1;
	struct FileHeader *Header;
	unsigned int t,x,y;
	unsigned long size, bytes_read;
	unsigned char Bmp, dummy, red, blue, green;
	int i,hindex,index,j;
	struct RGBQUAD Palette[256];
	unsigned long  *bgr_buff;
	char buff[50];
	(void) signal(SIGINT, ex_program);
	fp = fopen(fname,"rb");
	printf("\n File Name : %s",fname);
	Header = (struct FileHeader *) malloc (sizeof(struct FileHeader));
	if (!Header) {
		perror("Error:");
		exit(1);
	}
	if (!fp) {
		printf ("Error opening source file\r\n");
		perror ("Error");
		exit (1);
	}
	//printf("\nfilename: %s\n\n",fname);
	fread(&Type, sizeof(Type), 1, fp);
	bytes_read = sizeof(Type);
	fread(&Size, sizeof(Size), 1, fp);
	bytes_read += sizeof(Size);
	if ((fread(Header, sizeof(struct FileHeader), 1, fp)) == -1) {
		printf ("Error: Unable to read File header.\r\n");
		exit (1);
	}
	bytes_read += sizeof(struct FileHeader);
	while (bytes_read < Header->OffBits) {
		if (fread(&dummy,sizeof(dummy),1,fp)!=1) {
			printf("Error seeking to bitmap data\n");
			exit(-1);
		}
		++bytes_read;
	}
	size = Header->Width * Header->Height;
	printf ("BMP Width = %d\tBMP Height = %d\n", Header->Width, Header->Height);
	printf ("Bit Count = %d\n", Header->BitCount);
	index=0;
	if (Header->BitCount == 24) {
		bgr_buff = (unsigned long *) malloc (size * sizeof(unsigned long));
		for (i = 0; i < size; i++) {
			blue = fgetc(fp);
			green = fgetc(fp);
			red  = fgetc(fp);
			p1 = 0;
			p1 |= red;
			p1 = p1 << 16;
			pixel = p1;
			p1 = 0;
			p1 |= green;
			p1 = p1 << 8;
			pixel |= p1;
			p1 = 0;
			p1 |= blue;
			pixel |= p1;
			bgr_buff [index] = pixel;
			index ++;
		}
// At this point bgr_buff contains the RGB values for the pixels defined by height & width of the BMP file.
		hindex=0;
		y = Header->Height-1;
		while(y > 0) {
			for(x=0; x < Header->Width ; x++) {
				location = (x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) +(y+vinfo.yoffset) * finfo.line_length;
				*((unsigned long *)(fbp + location)) = bgr_buff [hindex] ;
				hindex++;
			}
			--y;
		}
		free(bgr_buff);
	}
	free(Header);
	printf ("\n[Done]\n\r");
	j++;
	fclose (fp);
	return( 0 );
}
/***** SHOW_BMP*****/

/***** SHOW_IMAGE*****/
int show_Image(char *img)
{
	unsigned long size, bytes_read;
	struct direct **files;
	char *pathname;
	int count=0,delay =0,i;
	pid=getpid();

	// Open the file for reading and writing
	fbfd = open("/dev/fb0", O_RDWR);
	if (!fbfd) {
		printf("Error cannot open framebuffer device.\n");
		exit(1);
	}

	// Get fixed screen information
	if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo)) {
		printf("Error reading fixed information.\n");
		exit(2);
	}

	// Get variable screen information
	if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
		printf("Error reading variable information.\n");
		exit(3);
	}
	printf("%dx%d, %dbpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel );

	screensize = 0;

	// Figure out the size of the screen in bytes
	screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

	// Map the device to memory
	fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED,fbfd, 0);

	if ((int)fbp == -1) {
		printf("Error failed to map framebuffer device to memory.\n");
		exit(4);
	}

	printf("\n* * * Inside show_image() * * *\n");
	show_bmp(img);

	munmap(fbp, screensize);
	close(fbfd);
	return 0;
}
/***** SHOW_IMAGE*****/

/***** GLCD *****/

/* Application : To test the functionaity of LCD Driver */
/***** TOUCH MAIN *****/
int touch_main()
{
	unsigned long size, bytes_read;
	int count=0,delay =0,i;
	int hindex=0, x, y, location=0;
	int sx, sy;

	int exp_dev = 0;
	int res = 0;
	char buff[256] = "Name";

	int fd, rb;

        struct input_event ev;
        struct input_absinfo ab;

	if ((fd = open("/dev/input/touchscreen0", O_RDONLY)) < 0) 
	{
		printf ("Error Opening the Device\n");
                return 1;
	}

	ioctl(fd, EVIOCGNAME(sizeof(buff)), buff);
        for (i = 0; i < 3; i++)
	{
        	rb=read(fd,&ev,sizeof(struct input_event));
		printf ("\nIN TOUCH MAIN");
		/*if ((ev.type == 1) && (ev.code == 330))
		{
			printf ("Touch Event = %d\n",ev.code);
		}*/
		if ((ev.type == 3) && (ev.code == 0))
		{
			//printf ("X Value  = %d\n",ev.value);
			sx=ev.value;
		}
		if ((ev.type == 3) && (ev.code == 1))
		{
			//printf ("Y Value  = %d\n",ev.value);
			sy=ev.value;
		}
		/*if ((ev.type == 3) && (ev.code == 24))
		{
			printf ("Touch Pressure  = %d\n",ev.value);
		}*/
		printf("\nSX = %d, SY = %d", sx, sy);
		/** RANK#1 **/
		if((sx>0 && sx<1800) && (sy>0 && sy<1800))
		{
			printf("\ncollege");
			show_Image("PGC.bmp");
			process_college(); //  change process //
		}
		if((sx>2200 && sx<4000) && (sy>0 && sy<1800))
		{
			printf("\nAdmission");
			show_Image("All_Branch.bmp");
			process_admission();  //  change process //
		}
		if((sx>0 && sx<1800) && (sy>2200 && sy<4000))
		{
			printf("\nHostel");
			show_Image("Hostel.bmp"); //  change process //
			process_hostel();
		}
		if((sx>2200 && sx<4000) && (sy>2200 && sy<4000))
		{
			printf("\nFest");
			show_Image("Fest.bmp");
			process_fest(); //  change process //
		}
	}//end of for loop
	return 0;
}
/***** TOUCH MAIN *****/

/***** TOUCH OPTIONS *****/
int touch(int index)
{
	unsigned long size, bytes_read;
	int count=0,delay =0,i;
	int hindex=0, x, y, location=0;
	int sx, sy;

	int exp_dev = 0;
	int res = 0;
	char buff[256] = "Name";

	int fd, rb;

        struct input_event ev;
        struct input_absinfo ab;

	if ((fd = open("/dev/input/touchscreen0", O_RDONLY)) < 0) 
	{
		printf ("Error Opening the Device\n");
                return 1;
	}

	ioctl(fd, EVIOCGNAME(sizeof(buff)), buff);
        for (i = 0; i < 3; i++)
	{
        	rb=read(fd,&ev,sizeof(struct input_event));
		printf ("After read\n");
		/*if ((ev.type == 1) && (ev.code == 330))
		{
			printf ("Touch Event = %d\n",ev.code);
		}*/
		if ((ev.type == 3) && (ev.code == 0))
		{
			//printf ("X Value  = %d\n",ev.value);
			sx=ev.value;
		}
		if ((ev.type == 3) && (ev.code == 1))
		{
			//printf ("Y Value  = %d\n",ev.value);
			sy=ev.value;
		}
		/*if ((ev.type == 3) && (ev.code == 24))
		{
			printf ("Touch Pressure  = %d\n",ev.value);
		}*/
		printf("\n\nSX = %d, SY = %d", sx, sy);
		/** RANK#1 **/
		if((sx>0 && sx<1800) && (sy>0 && sy<1800))
		{
			printf("\nRANK#1");
			count = 0;
		}
		if((sx>2200 && sx<4000) && (sy>0 && sy<1800))
		{
			printf("\nRANK#2");
			count = 1;		}
		if((sx>0 && sx<1800) && (sy>2200 && sy<4000))
		{
			printf("\nRANK#3");
			count = 2;
		}
		if((sx>2200 && sx<4000) && (sy>2200 && sy<4000))
		{
			printf("\nRANK#4");
			count = 3;
		}
		printf("\n * * * * * COUNT = %d, INDEX = %d",count, index);
		switch(index)
		{
			case 1:
				LED_Display(list_college[count].estd);              //  Second code Execution //
				CLCD_Display(list_college[count].name);
                                    if((count+1)==1)
                                     show_Image("PCE.bmp");
                                    else
                                     if((count+1)==2)
                                     show_Image("PIET.bmp");
                                    else
                                        if((count+1)==3)
                                     show_Image("PGI.bmp");
                                    else
                                       if((count+1)==4)
                                     show_Image("PU.bmp");
                                    
				break;
			case 2:
				LED_Display(list_admission[count].nos);    //  Third code Execution //
				CLCD_Display(list_admission[count].bname);
                                   if(count+1==1)
                                     show_Image("me.bmp");
                                    else
                                     if(count+1==2)
                                     show_Image("civ.bmp");
                                    else
                                        if(count+1==3)
                                     show_Image("cs.bmp");
                                    else
                                       if(count+1==4)
                                     show_Image("ee.bmp");
                                    
				break;
			case 3:
				LED_Display(list_hostel[count].nob);     //  Fourth code Execution //
				CLCD_Display(list_hostel[count].warden);
                                    if(count+1==1)
                                     show_Image("gurushikhar.bmp");
                                    else
                                     if(count+1==2)
                                     show_Image("gayatri.bmp");
                                    else
                                        if(count+1==3)
                                     show_Image("aravali.bmp");
                                    else
                                       if(count+1==4)
                                     show_Image("himalaya.bmp");
                                    
				break;
			case 4:
				LED_Display(list_fest[count].nod);   //  Fifth code Execution //
				CLCD_Display(list_fest[count].fname);

                                  // show country rank
                                   if(count+1==1)
                                     show_Image("arohan.bmp");
                                    else
                                     if(count+1==2)
                                     show_Image("ayam.bmp");
                                    else
                                        if(count+1==3)
                                     show_Image("yuvam.bmp");
                                    else
                                       if(count+1==4)
                                     show_Image("param.bmp");
                                    
				break;
			default:
				printf("\n Invalid Choice of Main Options\n");
		}

	}//end of for loop
	printf("\n HIT A KEY...");
	getchar();
	return 0;
}
/***** TOUCH OPTIONS *****/

/***** 7SEG LED INIT *****/
int LED_Init()
{
    /* open as blocking mode for 7Segment LED*/
    exp_seg = open(NODE_SEG, O_RDWR);
	if (exp_seg < 0){
		fprintf(stderr, "Open error: %s\n", NODE_SEG);
		return 1;
	}
	if (res=ioctl(exp_seg,SEG_7_LED_ON, NULL) < 0 ){
        	printf("%d---> Error in switching OFF the LED \r\n",res);
        	close(exp_seg);
                return 1;
	}
	return 0;
}
/***** 7SEG LED INIT *****/
/***** 7SEG LED DISPLAY *****/
int LED_Display(int index)
{
	if (res=ioctl(exp_seg,SEG_7_LED_WRITE, index) < 0 ){
		printf("%d---> Error in writing to the 7-Seg LED \r\n",res);
		close(exp_seg);
		return 1;
	}
	return 0;
}
/***** 7SEG LED DISPLAY *****/
/***** 7SEG LED CLEAR *****/
int LED_Clear()
{
	if (res=ioctl(exp_seg,SEG_7_LED_CLEAR, NULL) < 0 ){
        	printf("%d---> Error in Clearing the LED \r\n",res);
                close(exp_seg);
                return 1;
	}
	return 0;
}
/***** 7SEG LED CLEAR *****/
/***** 7SEG LED CLOSE *****/
int LED_Close()
{
        if (ioctl(exp_seg,SEG_7_LED_OFF, NULL) < 0 ){
        	printf("Error in switching OFF the LED \r\n");
                close(exp_seg);
                return 1;
	}
	return 0;
}
/***** 7SEG LED CLOSE *****/

/***** CLCD INIT *****/
int CLCD_Init()
{
    /* open as blocking mode for CLCD*/
    exp_clcd = open(NODE_CLCD, O_RDWR);
    if (exp_clcd < 0)
    {
        fprintf(stderr, "Open error: %s\n", NODE_CLCD);
	return 1;
    }
	if (ioctl(exp_clcd,LCD_ON, NULL) < 0)
	{
		printf("Error in Writing the LCD \r\n");
		close(exp_clcd);
		return 1;
	}
	if (ioctl(exp_clcd,LCD_CLEAR, NULL) < 0)
	{
		printf("Error in Clearing the LCD \r\n");
	     	close(exp_clcd);
	    	return 1;
	}
	return 0;
}
/***** CLCD INIT *****/
/***** CLCD DISPLAY *****/
int CLCD_Display(char *info)
{
	if (ioctl(exp_clcd,LCD_SET_HOME, NULL) < 0 )
        {
        	printf("Error in setting the cursor in home position \r\n");
		close(exp_clcd);
                return 1;
	}
	if (ioctl(exp_clcd,LCD_CURSOR_BLINK, NULL) < 0 )
        {
        	printf("Error in Curosr blinking \r\n");
                close(exp_clcd);
                return 1;
	}
	if (ioctl(exp_clcd,LCD_DATA_WRITE, info) < 0 )
        {
        	printf("Error in writing the LCD \r\n");
                close(exp_clcd);
                return 1;
	}
	return 0;
}
/***** CLCD DISPLAY *****/
/***** CLCD CLEAR *****/
int CLCD_Clear()
{
	if (ioctl(exp_clcd,LCD_CLEAR, NULL) < 0 )
        {
        	printf("Error in Clearing the LCD \r\n");
                close(exp_clcd);
                return 1;
	}
	return 0;
}
/***** CLCD CLEAR *****/
/***** CLCD CLOSE *****/
int CLCD_Close()
{
	if (ioctl(exp_clcd,LCD_OFF, NULL) < 0 )
        {
        	printf("Error in Switching OFF the LCD \r\n");
                close(exp_clcd);
                return 1;
	}
	return 0;
}
/***** CLCD CLOSE *****/

/*****  *****/
int process_college()
{
	int ch=0;
	do{
		touch(1);
		printf("\n1. Continue...(0 for YES, 1 for NO)");
		fflush(stdin);
		scanf("%d",&ch);
		LED_Clear();
		CLCD_Clear();
	} while(ch==0);
	return 0;
}
/***** process_college *****/

/***** process_admission *****/
int process_admission()
{
	int ch=0;
	do{
		touch(2);
		printf("\n2. Continue...(0 for YES, 1 for NO)");
		fflush(stdin);
		scanf("%d",&ch);
		LED_Clear();
		CLCD_Clear();
	} while(ch==0);
	return 0;
}
/***** process_admission *****/

/***** proce_hostel *****/
int process_hostel()
{
	int ch=0;
	do{
		touch(3);
		printf("\n3. Continue...(0 for YES, 1 for NO)");
		fflush(stdin);
		scanf("%d",&ch);
		LED_Clear();
		CLCD_Clear();
	} while(ch==0);
	return 0;
}
/***** process_hostel *****/

/***** process_fest *****/
int process_fest()
{
	int ch=0;
	do{
		touch(4);
		printf("\n4. Continue...(0 for YES, 1 for NO)");
		fflush(stdin);
		scanf("%d",&ch);
		LED_Clear();
		CLCD_Clear();
	} while(ch==0);
	return 0;
}
/***** process_fest *****/


int main ()
{
	int ch=0;                   // First code Execution  //
	LED_Init();
	CLCD_Init();
	do{
		show_Image("Home_Page.bmp");
		touch_main();
		printf("\n Continue for the main...(0 for YES, 1 for NO)");
		fflush(stdin);
		scanf("%d",&ch);
		LED_Clear();
		CLCD_Clear();
	} while(ch==0);
	LED_Close();
	CLCD_Close();
	return 0;
}
