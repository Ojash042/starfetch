#include "ANSI-color-codes.h"
#include "ascii_logo.c"

#include <errno.h>
#include <locale.h>
#include <math.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>
#include <wchar.h>

#define STARTING_LINES 0
#define STARTING_COLS 0
#define REFRESH_RATE 83333

typedef struct {
  char *key;
  char *value;
} OSReleaseData;

typedef struct {
  char deviceName[1024];
  char hostName[1024];
  char prettyName[1024];
  char systemName[1024];
  char kernelVersion[1024];
  char uptime[1024];
  char memoryUsage[1024];
} SystemDetails;

typedef struct{
  long mem_total;
  long mem_available;
  long mem_used;
} MemInfo;


OSReleaseData *parseOSData(int *total_items);


void getScreenSize(int *rows, int *cols) {
  struct winsize w;

  ioctl(0, TIOCGWINSZ, &w);

  *rows = w.ws_row;
  *cols = w.ws_col;

  return;
}

/**
 * @breif gets the value belonging to the key in OSReleaseData type
 *
 * This function searches for the value belonging to the key that
 * is parsed from the os_release file
 *
 * @params os_release
 * @params total_items
 * @params key
 **/
char *getOSReleaseVal(OSReleaseData *os_release, int *total_items, char *key) {
  for (int i = 0; i < *total_items; i++) {
      if (strcmp((os_release + i)->key, key) == 0) {
      return (os_release + i)->value;
    }
  }
  return "";
}


char* parseMemInfo(){
  MemInfo *meminfo = (MemInfo *)malloc(sizeof(MemInfo));
  FILE *meminfo_file = fopen("/proc/meminfo", "r");
  if(meminfo == NULL){
    fprintf(stderr,"%sError opening file%s\n", BRED, CRESET);
    exit(1);
  }

  char buffer[1024];
  char *delimeter = ": ";

  while(fgets(buffer, sizeof(buffer),meminfo_file )){
    char *temp = strtok(buffer, delimeter); 
    char *key = strdup(temp);

    if(strcmp(key, "MemTotal") == 0 || strcmp(key, "MemAvailable") == 0){
      temp = strtok(NULL, delimeter);
    }

    char *value = strdup(temp);

    if(strcmp(key, "MemTotal") == 0){
      meminfo->mem_total = atol(value) / 1024;
    }
   
    if(strcmp(key, "MemAvailable") == 0){
      meminfo-> mem_available =  atol(value) / 1024;
    }
    
  }
  fclose(meminfo_file);
  memset(buffer, 0, sizeof(buffer));

  meminfo->mem_used = meminfo->mem_total - meminfo->mem_available;

  sprintf(buffer, "%ld/%ld (%.2f%% used)", meminfo->mem_used, meminfo->mem_total, 
           ( (float) meminfo->mem_used / (float) meminfo->mem_total) * 100);

  char *returning_var = malloc(sizeof(buffer));
  strcpy(returning_var, buffer);
  return returning_var;
}


OSReleaseData *parseOSData(int *total_items) {
  FILE *os_releases;
  os_releases = fopen("/etc/os-release", "r");
  if (os_releases == NULL) {
    printf("Error opening file\n");
    exit(1);
  }

  int total_no_of_lines = 0;
  int line_no = 0;
  char buffer[12][1024];
  char buf[1024];
  char *temp;
  char *delimeter = "=\"";
  int c;

  while ((c = fgetc(os_releases)) != EOF) {
    // Checks whether the character is new line char and adds an int value based
    // on the integer returned either 1 or 0
    total_no_of_lines += (c == '\n');
  }

  *total_items = total_no_of_lines;
  OSReleaseData *dict =
      (OSReleaseData *)malloc(total_no_of_lines * sizeof(OSReleaseData));

  rewind(os_releases);

  for (int i = 0; i < total_no_of_lines; i++) {
    (dict + i)->key = NULL;
    (dict + i)->value = NULL;
  }

  while (fgets(buf, sizeof(buf), os_releases) != NULL) {
    temp = strtok(buf, delimeter);

    (dict + line_no)->key = strdup(temp);
    temp = strtok(NULL, delimeter);
    (dict + line_no)->value = strdup(temp);
    line_no++;
  }
  fclose(os_releases);
  return dict;
}

char *appendTimeValues( char* buffer, int time_data, char* unit){
  if(time_data>0){
    char *plural = time_data>1 ? "s" :"";
    if(strcmp(buffer, "") == 0){
      sprintf(buffer, "%s%d %s%s",buffer, time_data, unit, plural);
    }
    else
      sprintf(buffer, "%s %d %s%s",buffer, time_data, unit, plural);
  }
  return buffer;
}

char *humanizeTime(long seconds){ 

  char buffer[256];
  memset(buffer, 0, sizeof(buffer));

  int second = seconds % 60;
  int mins = (seconds / 60);
  int hours = (mins / 60);
  int days = (hours / 24); 
  int years = days / 365;

  appendTimeValues(buffer, years, "year");
  appendTimeValues(buffer, days % 365, "day");
  appendTimeValues(buffer, hours % 24, "hour");
  appendTimeValues(buffer, mins % 60, "minute");
  appendTimeValues(buffer, second % 60, "second");

  char *return_value = malloc(sizeof(buffer));
  strcpy(return_value, buffer);
  return return_value; 
}

SystemDetails *getSystemDetails() {
  char buf[246];
  SystemDetails *sysDetails = (SystemDetails *)malloc(sizeof(SystemDetails));

  int total_items = 0;
  OSReleaseData *os_release = parseOSData(&total_items);

  char *pretty_name = getOSReleaseVal(os_release, &total_items, "PRETTY_NAME");
  if (strcmp(pretty_name, "") == 0) {
    printf("%sError: %sCould Not Find pretty_name\n%sExiting%s\n", BRED, CRESET,
           BRED, CRESET);
    exit(1);
  }

  struct utsname utsname;
  errno = 0;
  if (uname(&utsname) < 0) {
    perror("uname");
    exit(EXIT_FAILURE);
  }

  char hostNameBuf[1028];
  gethostname(buf, 128);
  
  strcpy(sysDetails-> hostName ,buf);
  strcpy(sysDetails->deviceName, utsname.nodename);
  memset(buf, 0, sizeof(buf));

  strcpy(sysDetails->prettyName , pretty_name);
  
  strcpy(sysDetails->kernelVersion, utsname.release);
  strcpy(sysDetails->systemName, utsname.sysname); 

  memset(buf, 0, sizeof(buf));
    
  return sysDetails;
}

int main() {
  int total_no_of_lines = 0;

  SystemDetails *sysDetails = getSystemDetails();
  int rows, cols;

  time_t timer;
  struct tm *lTime;

  setlocale(LC_ALL, "");
  initscr();
  clear();
  curs_set(0);

  start_color();
  use_default_colors();
  nodelay(stdscr, TRUE);
  refresh();

  init_pair(1, COLOR_YELLOW, -1);
  init_pair(2, COLOR_BLUE, -1);

  init_pair(3, COLOR_MAGENTA, -1);
  init_pair(4, COLOR_GREEN, -1);

  init_pair(5, COLOR_BLACK, -1);
  init_pair(6, COLOR_RED, -1);
  init_pair(7, COLOR_CYAN, -1);
  init_pair(8, COLOR_WHITE, -1);

  getScreenSize(&rows, &cols);
  clear();
  int ch = -1;

  
  while (ch != 'q') {

    timer = time(NULL);
    lTime = localtime(&timer);

    for (int i = 0; i < (sizeof(linuxLogo) / sizeof(linuxLogo[0])); i++) {
      mvprintw(i, 10, "%s\n", linuxLogo[i]);
    }
    const int starting_col = ceil((float)cols / 2) + 10;
    
    move(5, starting_col + 5);
    attron(COLOR_PAIR(1) | A_BOLD);
    printw("%s", sysDetails->hostName);
    attroff(COLOR_PAIR(1)| A_BOLD);
    printw(" @ ");
    attron(COLOR_PAIR(2) | A_BOLD);
    printw("%s\n", sysDetails->deviceName);
    attroff(COLOR_PAIR(2) | A_BOLD);

    move(6,starting_col + 5);
    printw("---------------");

    move(8, starting_col + 5);
    attron(COLOR_PAIR(3) | A_BOLD);
    printw(" ");
    attroff(COLOR_PAIR(3) | A_BOLD);
    move(8, starting_col + 10);
    attron(COLOR_PAIR(4));
    printw("%s",sysDetails->prettyName);
    attroff(COLOR_PAIR(4));
    
    move(9, starting_col + 5);
    attron(COLOR_PAIR(3) | A_BOLD);
    printw(" ");
    attroff(COLOR_PAIR(3) | A_BOLD);
    move(9, starting_col + 10);
    attron(COLOR_PAIR(4));
    printw("%s %s",sysDetails->systemName, sysDetails->kernelVersion);
    attroff(COLOR_PAIR(4));


    move(10, starting_col + 5);
    attron(COLOR_PAIR(3) | A_BOLD);
    printw(" ");
    attroff(COLOR_PAIR(3) | A_BOLD);

    move(10, starting_col + 10);
    struct sysinfo s_info;
    sysinfo(&s_info);
    char *h_time = humanizeTime(s_info.uptime);
    char human_time[128];
    strcpy(human_time, h_time);

    attron(COLOR_PAIR(4));
    printw("%s\n", human_time);
    attroff(COLOR_PAIR(4));

    move(11, starting_col + 5);
    attron(COLOR_PAIR(3) | A_BOLD);
    printw("󰍛 ");
    attroff(COLOR_PAIR(3) | A_BOLD);
    move(11, starting_col + 10);
    attron(COLOR_PAIR(4));
    printw("%s", parseMemInfo());
    attroff(COLOR_PAIR(4));

    time_t timer;
    struct tm *time_info;
    time(&timer);
    time_info = localtime(&timer);
    move(12, starting_col + 5);
    attron(COLOR_PAIR(3) | A_BOLD);
    printw("󰥔 ");
    attroff(COLOR_PAIR(3) | A_BOLD);
    move(12, starting_col + 10);
    attron(COLOR_PAIR(4));
    printw("%s", asctime(time_info));
    attroff(COLOR_PAIR(4));
    
    move(13, starting_col + 5);
    attron(COLOR_PAIR(3) | A_BOLD);
    printw(" ");
    move(13, starting_col + 10);
    attron(COLOR_PAIR(5));
    printw("████");
    attroff(COLOR_PAIR(5));
    attron(COLOR_PAIR(6));
    printw("████");
    attroff(COLOR_PAIR(6));
    attron(COLOR_PAIR(4));
    printw("████");
    attroff(COLOR_PAIR(4));
    attron(COLOR_PAIR(1));
    printw("████");
    attroff(COLOR_PAIR(1));
    attron(COLOR_PAIR(2));
    printw("████");
    attroff(COLOR_PAIR(2));
    attron(COLOR_PAIR(3));
    printw("████");
    attroff(COLOR_PAIR(3));
    attron(COLOR_PAIR(7));
    printw("████");
    attroff(COLOR_PAIR(7));
    attron(COLOR_PAIR(8));
    printw("████");
    attroff(COLOR_PAIR(8));
       
    refresh();
    usleep(REFRESH_RATE);
    ch = getch();
  }
  endwin();
  printf("\n");
  return 0;
}
