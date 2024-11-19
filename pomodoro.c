#define MINIAUDIO_IMPLEMENTATION
#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include "./src/miniaudio.h"
#include "./src/config.h"


WINDOW *win;
unsigned int pause_time;
unsigned int long_pause_time;
unsigned char long_pause_interval;
unsigned int work_time;
char* notification_audio_file;

char* home_dir(char* input, int len_string)
{
  char* path_output;

  path_output = malloc(sizeof(char) * len_string);

  strcpy(path_output, getenv("HOME"));
  strcat(path_output, input);
  return path_output;
}

void parsing_audio_file_config(char* audio_config_path)
{
  int count;
  char* output;
  int len_output;
  char* temp_config_path;

  count = 0;
  
  if ((*(audio_config_path + 0) == '~'))
  {
    len_output =  strlen(getenv("HOME")) + strlen(audio_config_path) + 25 ;

    temp_config_path = malloc(sizeof(char) * (strlen(audio_config_path) ));

    while (count < (strlen(audio_config_path) ))
    {
      temp_config_path[count]  = audio_config_path[count + 1];
      count++;
    }
      
    notification_audio_file = malloc(sizeof(char) * len_output);
    notification_audio_file = home_dir(temp_config_path, len_output);
    
  }
}

void config_file_treatment(char* file_path)
{
  CfgError err;
  CfgEntry *entries;
  int parsing_state;
  char* parsing_audio_conf_path;

  entries = malloc(64 * sizeof(CfgEntry));
  Cfg cfg = {.entries = entries, .capacity = 64};

  parsing_state = cfg_parse_file(file_path, &cfg, &err);
  
  if (parsing_state != 0)
  {
    cfg_fprint_error(stderr, &err);
    free(entries);
    exit(1);
  }
  pause_time = cfg_get_int(&cfg, "pause.time", 5);
  long_pause_time = cfg_get_int(&cfg, "long_pause.time", 15);
  long_pause_interval = cfg_get_int(&cfg, "long_pause.interval", 4);
  work_time = cfg_get_int(&cfg, "work.time", 25);
  parsing_audio_conf_path = cfg_get_string(&cfg, "notification_audio.filepath" ,"~/.config/simple-pomodoro-cli/audio/initial_d.wav");
  parsing_audio_file_config(parsing_audio_conf_path);
  free(entries);
  
}

void config_file_check()
{
  int config_file;

  char* file_path;
  int path_len;
  
  path_len = strlen(getenv("HOME")) + strlen("/.config/simple-pomodoro-cli/config.cfg") + 1;
  file_path = malloc(sizeof(char) * path_len);


  strcpy(file_path, home_dir("/.config/simple-pomodoro-cli/config.cfg",path_len));
  if (access(file_path, R_OK|W_OK) == 0)
  {
    config_file_treatment(file_path);
  }
  else 
  {
    
    int create_file = creat(file_path, 0666);
    int change = chmod(file_path, 0666);

    FILE* fileptr = fopen(file_path, "w");
    fprintf(fileptr, "pause.time: 5\nlong_pause.time: 15\nlong_pause.interval: 4\nwork.time: 25\nnotification_audio.filepath: %c~/.config/simple-pomodoro-cli/audio/initial_d.wav%c", '"','"'  );
    fclose(fileptr);
    config_file_treatment(file_path);
  }
  
}

void config_check()
{
  DIR* config_dir;
  char* folder_path;
  int path_len;
  
  path_len = strlen(getenv("HOME")) + strlen("/.config/simple-pomodoro-cli") + 1;
  folder_path = malloc(sizeof(char) * path_len);

  strcpy(folder_path,home_dir("/.config/simple-pomodoro-cli",path_len));

  config_dir = opendir(folder_path);
  if (config_dir)
  {
    config_file_check();
  }
  else if (errno == ENOENT) //if folder doesnt exist
  {
    int mkdir_state = mkdir(folder_path, 0777);
    config_file_check();
  }
  else
  {
    fprintf(stderr,"%s", strerror(errno));
    exit(1);
  }

}

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
  ma_decoder* pDecoder = (ma_decoder*)pDevice->pUserData;
  if (pDecoder == NULL) {
      return;
  }

  ma_decoder_read_pcm_frames(pDecoder, pOutput, frameCount, NULL);

  (void)pInput;
}


void getch_w_exit()
{
  char input;

  input = getch();

  if ((input == 113) || (input == 27))
  {
    endwin();
    exit(1);
  }
}

void  printw_center(char* string, char line, unsigned char format)
{
  unsigned char len_string;
  len_string = strlen(string);

  if ((len_string % 2) == 0)
  {
    len_string = len_string / 2;
  }
  else
  {
    len_string = len_string / 2 + 1 ;
  }

  move(LINES / 2 + line, COLS / 2 - len_string);

  switch (format) 
  {
    case 0:
      break;
    case 1:
      attron(A_STANDOUT | COLOR_PAIR(1));
      break;
    case 2:
      attron(A_STANDOUT | COLOR_PAIR(2));
      break;
  }

  
  
  printw(string);
  switch (format) 
  {
    case 0:
      break;
    case 1:
      attroff(A_STANDOUT | COLOR_PAIR(1));
      break;
    case 2:
      attroff(A_STANDOUT | COLOR_PAIR(2));
      break;
  }
  
}


int session_period(int minutes_max, char* label)
{


  int minutes;
  int seconds;
  char interface_text[8];
  char end_message[20];
  char min_string[3];
  char min_output[3];
  char sec_string[3];
  char sec_output[3];
  int period_time;
  ma_result result;
  ma_decoder decoder;
  ma_device_config deviceConfig;
  ma_device device;



  result = ma_decoder_init_file(notification_audio_file, NULL, &decoder);
  if (result != MA_SUCCESS) {
      endwin();
      fprintf(stderr, "Could not load file: %s", notification_audio_file);
      exit(1);
  }

  deviceConfig = ma_device_config_init(ma_device_type_playback);
  deviceConfig.playback.format   = decoder.outputFormat;
  deviceConfig.playback.channels = decoder.outputChannels;
  deviceConfig.sampleRate        = decoder.outputSampleRate;
  deviceConfig.dataCallback      = data_callback;
  deviceConfig.pUserData         = &decoder;

  if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS) {
      endwin();
      fprintf(stderr,"Failed to open playback device.");
      ma_decoder_uninit(&decoder);
      exit(1); 
  }

  
  nodelay(win, TRUE);
  period_time = 0;
  minutes = 0;
  seconds = 0;


  while (minutes < minutes_max)
  {

    getch_w_exit();

         //FLUSHING
    sec_string[0] = '\0';
    min_string[0] = '\0';
    sec_output[0] = '\0';
    min_output[0] = '\0';
    interface_text[0] = '\0';

    //OUTPUT

    sprintf(min_string,"%d", minutes);
   
    if (min_string[1] == '\0')
    {
      sprintf(min_output,"0%d", minutes);
    }
    else 
    {
      sprintf(min_output, "%d", minutes);
    }
    

    sprintf(sec_string,"%d", seconds);
    if (sec_string[1] == '\0')
    {
      sprintf(sec_output,"0%d", seconds);
    }
    else 
    {
      
      sprintf(sec_output,"%d", seconds);
    }
    
    clear();
    printw_center(label, 0, 0);
    sprintf(interface_text,"%s:%s", min_output,sec_output);
    printw_center(interface_text, 2, 2);
    refresh();


    
    sleep(1);
    
    

    period_time += 1;
    minutes = period_time / 60;
    seconds = period_time % 60;


  }
 
  clear();

  nodelay(win, FALSE);
  if (ma_device_start(&device) != MA_SUCCESS) {
    endwin();
    fprintf(stderr, "Failed to start playback device.");
    ma_device_uninit(&device);
    ma_decoder_uninit(&decoder);
    exit(1);
  }
  sprintf(end_message, "%s is finished", label);
  printw_center(end_message, 0, 1);
  refresh();
  getch();
  ma_device_uninit(&device);
  ma_decoder_uninit(&decoder);
}

void config()
{

}


int  session(unsigned char session_max, unsigned char actual_session)
{
  int next_session;

  next_session = actual_session + 1;
  if (actual_session == session_max )
  {
    clear();
    printw_center("Good boy =3", 0, 2);
    refresh();
    getch();
    return 0;
  }

  session_period(work_time, "Work");
  nodelay(win, FALSE);
  getch_w_exit();
  
  clear();
  if ((actual_session % long_pause_interval) == 0)
  {
    session_period(pause_time, "Pause");
  }
  else 
  {
    session_period(long_pause_time, "Pause");
  }
  
  getch_w_exit();

  return session(session_max, next_session);
}
void help()
{
  printf("\nUsage: simple-pomodoro-cli number_of_session");
}


int main(int argc, char *argv[])
{     
  if (argc != 2)
  {
    fprintf(stderr,"You provided too much or too few arguments.\n");
    help();
    exit(1);
  }
  if ((argv[2] == "--help") || (argv[2] == "-h") || (argv[2] == "-help"))
  {
    help();
    exit(1);
  }

  config_check();

  int argument;



  win = initscr();
  curs_set(0);
  noecho();


   if (has_colors() == FALSE)
  {
    endwin();
    fprintf(stderr, "This terminal does not support color");
    exit(1);
  }
  start_color();
  init_pair(1, COLOR_WHITE, COLOR_RED);
  init_pair(2, COLOR_MAGENTA, COLOR_WHITE);

  argument = atoi(argv[1]);

  printw_center("POMODORO TIMER", 0, 1);
  printw_center("(Press ENTER to start)", 2, 0);
  refresh();
  getch_w_exit();
  clear();
  nodelay(win, TRUE);
  session(argument,0);
  refresh();
  getch();
  endwin();
  free(notification_audio_file);


  return 0;
} 
