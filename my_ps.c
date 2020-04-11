#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <sys/sysinfo.h>
#include <math.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <stdbool.h>

//FUNCTION PROTOTYPES
char* lCode(long);
void printPID(long);
long get_uptime();
char* get_current_date();
long get_start_time(long);
char* printVmLck(long);
char* minorCodes(long);
long get_TIME(long);
double get_CPU_USAGE(long);
char* getUser(uid_t);
int printUID(long);
void printPID(long);
char printState(long);
char* lCode(long);
int printMem(long);
char* printcmd(long);
char* cmd_arguments(long);

//checks if a given string is a number, used for memory checks of strings
bool isNumber(char* s) { 
    int i;
    for (i = 0; i < strlen(s); i++) 
        if (isdigit(s[i]) == false) { 
            return false; 
	}  

    return true; 
} 

//gets the time since last system boot in seconds 
long get_uptime() {
    struct sysinfo s_info;
    int error = sysinfo(&s_info);
    if(error != 0) { //error code 
        printf("error = %d\n", error);
    }
    return s_info.uptime;
}

//gets the current date, used to match a given process start date
char* get_current_date() {
    time_t t;
    struct tm *tmp ;
    char MY_TIME[50];
    time( &t );
    char* temp = malloc(sizeof(char)*sizeof(MY_TIME));
    memset(temp, '\0', sizeof(temp));
    tmp = localtime(&t);

    // using strftime to display time
    strftime(MY_TIME, sizeof(MY_TIME), "%b%d", tmp);
    strcpy(temp, MY_TIME);
    return temp;
    free(temp);
}

//gets the start time of a process
long get_start_time(long pid) {
    long uptime = get_uptime(); //calls system time
    char path[40], line[1024];
    FILE* statusf;
    snprintf(path, 40, "/proc/%ld/stat", pid); //get pathname

    statusf = fopen(path, "r");
    if(!statusf)
        return;
    
    fgets(line, 1024, statusf);
    char* pch = strtok(line, " "); //tokenize input
    int i = 0;
    int starttime;
    while (pch != NULL) {
	i++;
    	pch = strtok (NULL, " ");
	if(i == 21) { //check for starttime value
		starttime = atoi(pch); //convert to integer
	}
    }
    long starttime_result = (starttime)/sysconf(_SC_CLK_TCK); //convert to seconds instead of clock ticks

    fclose(statusf);
    
    //The epoch time a process was created
    long result = uptime - starttime_result;    
    return result;
  }

//Vmlck for "L" minor code
char* printVmLck(long pid) {
    char path[40], line[1024], *p;
    FILE* statusf;
    char* vmlck_state = ""; //temp
    int vmlck  = 0;
    snprintf(path, 40, "/proc/%ld/status", pid); //path

    statusf = fopen(path, "r");
    if(!statusf)
        return;

    while(fgets(line, 1024, statusf)) {
        if(strncmp(line, "VmLck:", 6) != 0) //check for vmlck area
            continue;
        // Ignore "State:" and whitespace
        p = line + 5;
        while(isspace(*p)) ++p;
        p[strlen(p)-3] = '\0';
        vmlck = atoi(p); //only gets the value
    }
    fclose(statusf);
    if(vmlck != 0) {
        vmlck_state = "L"; //set code
    }
    return vmlck_state;
}

//gets various minors codes of the different processes
char* minorCodes(long pid) {
    FILE* statusf;
    char* m_codes = malloc(5*sizeof(char)); //mem management
    memset(m_codes, '\0', sizeof(m_codes));
    int i  = 0;
    char path[40], line[1024];
    snprintf(path, 40, "/proc/%ld/stat", pid);

    statusf = fopen(path, "r");
    if(!statusf)
        return;

    //values to parse and look for
    int s_id = 0;
    int fg_id = 0;
    int nice_value = 0;
    int priority = 0;
    int p_id = 0;

    fscanf(statusf,"%ld", &p_id);
    fgets(line, 1024, statusf);

    char* pch = strtok(line, " "); //string tokenize the values
    while (pch != NULL) {
        i++;
        pch = strtok (NULL, " ");
        if(i == 4) {
                s_id = atoi(pch); //session id
        }
         if(i == 6) {
                fg_id = atoi(pch); //foreground group id
        }
        if(i == 16) {
                priority = atoi(pch);  //priority
        }
         if(i == 17) {
                nice_value = atoi(pch); //niceness
        }
    }
     if((nice_value < priority) & nice_value > 0) {
            strcat(m_codes, "N");  //nice
     }
     if(p_id == s_id) {
            strcat(m_codes, "s"); //session leader
     }
      strcat(m_codes, printVmLck(pid)); //puts codes together
      strcat(m_codes, lCode(pid));
     if(priority == 0 && nice_value < 0) {
         strcat(m_codes, "<"); //high priority
     }
     if(p_id == fg_id) {
     	    strcat(m_codes, "+"); //foreground
     }
     return m_codes;
     free(m_codes);
}

//Gets the time that a process has been running (not start time)
long get_TIME(long pid) {
    char path[40], line[1024];
    FILE* statusf;
    snprintf(path, 40, "/proc/%ld/stat", pid);

    statusf = fopen(path, "r");
    if(!statusf)
        return;

    fgets(line, 1024, statusf);
    char* pch = strtok(line, " ");
    int i = 0;
    int utime;
    int stime;
    
    while (pch != NULL) {
        i++;
        pch = strtok (NULL, " ");
        if(i == 13) {
                utime = atoi(pch); //user time
        }
	if(i == 14) {
                stime = atoi(pch); //system time
        }
     }
	
    //Calculating the total CPU time of a given process
    int total_time = utime + stime;   
    fclose(statusf);
    return total_time;
}

//function that computes the cpu usage of a process
double get_CPU_USAGE(long pid) {
    long process_time = get_TIME(pid); //gets time process has been running
    long start_time = get_start_time(pid); //gets creation time of process
    double cpu_usage = 100 * ((double)process_time / (double)start_time); //math
    return cpu_usage;
}

//Gets the user of a process given a corresponding UID
char* getUser(uid_t uid) {
    struct passwd* pws;
    pws = getpwuid(uid);
    return pws->pw_name;
}

//Gets the UID of a process
int printUID(long pid) {
    char path[40], line[1024], *p;
    FILE* statusf;
    int uid  = 0;
    snprintf(path, 40, "/proc/%ld/status", pid);

    statusf = fopen(path, "r");
    if(!statusf)
        return;

    while(fgets(line, 1024, statusf)) {
        if(strncmp(line, "Uid:", 3) != 0) //checks for uid
            continue;
        // Ignore "State:" and whitespace
        p = line + 5;
        while(isspace(*p)) ++p;
	p[strlen(p)-3] = '\0';
	uid = atoi(p);
    }
    fclose(statusf);
    return uid;
}

//simply prints a given pid
void printPID(long pid) {
    printf("%d\n", pid);
}

//prints the main states and concats minors codes
char printState(long pid) {
    char path[40],line[1024], *p;
    char state;
    FILE* statusf;
    int counter = 0;
    snprintf(path, 40, "/proc/%ld/status", pid);
    statusf = fopen(path, "r");
    if(!statusf)
        return;

    while(fgets(line, 1024, statusf)) {
        if(strncmp(line, "State:", 6) != 0) //check for state
            continue;
        // Ignore "State:" and whitespace
        p = line + 7;
        while(isspace(*p)) ++p;
	state = p[0];
    }
    fclose(statusf);
    return state;
}

//multithreaded minor code
char* lCode(long pid) {
    char path[40];
    char* mthread_state = "";
    snprintf(path, 40, "/proc/%ld/task", pid);
    int directory_count = 0;
    DIR * dirp;
    struct dirent * entry;
    dirp = opendir(path); 
    
    //if there is more than 1 directory in the task folder of proc for a pid, it is multithreaded
    while ((entry = readdir(dirp)) != NULL) {
    	if (entry->d_type == DT_DIR) { // If the entry is a regular directory 
         	directory_count++;
    	}
    }
    directory_count -=2; // . & ..
    if(directory_count > 1 || directory_count < 0) { mthread_state = "l"; } //checks if directories >1, adds code if so
    closedir(dirp);
    return mthread_state;
}

//gets resident set size
int printVmRSS(long pid) {
    char path[40], line[1024], *p, *x;
    FILE* statusf;
    int counter = 0;
    snprintf(path, 40, "/proc/%ld/status", pid);
    int number = 0;

    statusf = fopen(path, "r");
    if(!statusf)
        return;

    while(fgets(line, 1024, statusf)) {
        if(strncmp(line, "VmRSS:", 6) != 0) //checks for set size
            continue;

        // Ignore "State:" and whitespace
        p = line + 7;
        while(isspace(*p)) ++p;
	p[strlen(p)-2] = '\0';
	number = atoi(p);
    }
    fclose(statusf);
    return number;
}

//gets total memory on given machine
int printMem(long pid) {
    char path[40], line[1024], *p;
    FILE* statusf;
    snprintf(path, 40, "/proc/meminfo"); //checks meminfo file
    int num = 0;

    statusf = fopen(path, "r");
    if(!statusf)
        return;

    while(fgets(line, 1024, statusf)) {
        if(strncmp(line, "MemTotal:", 9) != 0) //checks for memtoal
            continue;

	p = line + 9;
        while(isspace(*p)) ++p;
	p[strlen(p)-3] = '\0';
        num = atoi(p);
    }
	fclose(statusf);
	return num;
}

//gets the cmd argument of ONLY PIDS that have relevant entries in the cmdline file
char* cmd_arguments(long pid) {
    char path[40];
    snprintf(path, 40, "/proc/%ld/cmdline", pid);
    char buffer[4096];

    //using old school methods for this, better for checking null spacing of cmdline file thanks :)
    int fd = open(path, O_RDONLY);
    int nbytesread = read(fd, buffer, 4096);
    char *end = buffer + nbytesread;
    char* p;
    int num_words  = 0;
    int i = 0;
    for (p = buffer; p < end; /**/) {
        while(*p++); // skip until start of next 0-terminated section in file
        num_words++; //gets number of words being parsed
    }
    char* a[num_words]; //array of words
    int argument_length = 0;
    for (p = buffer; p < end; /**/) {
        a[i] = p;
	argument_length += strlen(a[i]); //gets number of bytes
        strcpy(a[i],p); //makes 1 array of strings
        while(*p++);
        i++;
    }
  
    int j;
    char* arguments = (char*)malloc(sizeof(char)*argument_length+1);
    memset(arguments, 0, sizeof(arguments));
    int length = sizeof(a) / sizeof(a[0]);
    for(j = 0; j < length; j++) {
	strcat(arguments,a[j]); //creates 1 string
    }
    close(fd);
    return arguments;
    free(arguments);
}

//for processes that do not have cmdline entries, simply get executable name
char* printcmd(long pid) {
    char path[40];
    char* temp = NULL;
    snprintf(path, 40, "/proc/%ld/cmdline", pid);
    FILE *fp = fopen(path, "r");

    if(!fp) {
       return;
    }
 
    char line[1024];
    fgets(line, sizeof(line), fp);
    int num = atoi(line);
    int i = 1;

    if((isNumber(line))) { //checks for garbage values from cmdline file
        int i = 0;
        int PID = 0;
	char path2[40], line2[1024];
        FILE* statusff;
        snprintf(path2, 40, "/proc/%ld/stat", pid);
        statusff = fopen(path2, "r");
        if(!statusff)
            return;
      
        fscanf(statusff, "%d %s", &PID, line2); //scan for executable name, and mem managament
        temp = (char*)malloc(sizeof(char)*strlen(line2));
        //memset(temp, 0, sizeof(temp));
        strcpy(temp, line2);
	temp[0] = '[';
	temp[strlen(temp)-1] = ']'; //make output similar to ps aux with brackets
        return temp;
        fclose(statusff);
	free(temp);
    }
     return cmd_arguments(pid);
}

//gets the virutal memory size
int printVmSize(long pid) {
    char path[40], line[1024], *p;
    FILE* statusf;
    int value = 0;
    snprintf(path, 40, "/proc/%ld/status", pid);

    statusf = fopen(path, "r");
    if(!statusf)
        return;
     
      while(fgets(line, 1024, statusf)) {
        if(strncmp(line, "VmSize:", 6) != 0) //checks for vmsize
            continue;
      
        // Ignore "State:" and whitespace
        p = line + 7;
        while(isspace(*p)) ++p;
	p[strlen(p)-2] = '\0';
	value = atoi(p);
        fclose(statusf);
    }
    return value;
}

//simply print ?
char* TTY() { 
    char* tty = "?";
    return tty; 
}

int main(int argc, char** argv) {     

    char* current_date = get_current_date(); //gets current date, used to compare for START

    //HEADING FOR PS AUX
    printf("USER             PID      %%CPU     %MEM         VSZ         RSS      TTY    STAT      START        TIME     COMMAND\n");
    
    //opens the proc 
    DIR* proc = opendir("/proc");
    struct dirent* ent;
    long pid;
    if(proc == NULL) {
        fprintf(stderr,"error\n");
        return 0;
    }

    //reads proc directory
    while((ent = readdir(proc)) != NULL) {
        if(!isdigit(*ent->d_name)) //checks if it is not a numeric folder
            continue;
        pid = strtol(ent->d_name, NULL, 10);

        int uidNum = printUID(pid); //gets uid

        //converts epoch time to get time of a running process (not created time)
        double pid_time = get_TIME(pid)/sysconf(_SC_CLK_TCK); // in seconds
        double hours = pid_time / 60;
        int number = (int)hours;
        double decimal = (10 * hours - 10 * number)/10;
        double minutes  = decimal * 60;
        int min = (int)(round(minutes));

        //gets cpu usage
        double cpu = get_CPU_USAGE(pid);

        //gets vmsize, vmrss, and memtotal
        int vsz = printVmSize(pid);
        int rss = printVmRSS(pid);       
        int mem_total = printMem(pid);

        //gets memory percentage
        double percent_num = 100* ((double)rss / (double) mem_total);

        //gets uid and pid
        int uid = printUID(pid);      
        char* userName = getUser(uid);
      
        //gets ?, state of process, minor codes, and cmd line arguments
        char* tty = TTY();
        char state = printState(pid);
        char* m_codes = minorCodes(pid);
        char* cmd = printcmd(pid);        

        //PROCESS CREATION TIME
        time_t start = time(NULL) - get_start_time(pid);
        struct tm *tm = localtime(&start);
        char date[20];
        strftime(date, sizeof(date), "%b%d", tm);

	//check if it was printed today, then print time of process
	if(strcmp(date, current_date) == 0) {
		strftime(date, sizeof(date), "%H:%M", tm);
	}

        //ps aux itself with justified format, tried my best to make it look similar!
        printf("%-15s %4d      %-0.1f%%     %0.1f%%  %10d  %10d      %-3s    %c%-3s      %-10s %5d:%02d   %-10s\n", userName, pid, cpu/100, percent_num, vsz, rss, tty, state, m_codes, date, number, min, cmd);

   }
    closedir(proc); //close proc directory and we are done!
}
