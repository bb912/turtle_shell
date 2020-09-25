// Brett Bissey
// Operating Systems September 24 2020
// Dr. Boloni
// to compile: $ gcc shell.c
// to run: $ ./a.out
// for help in shell: # help

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#define DEBUG 0

#define FILE_MAX 100
#define TOKENS_SIZE 20
#define BACKGROUND 10
#define MY_STR_LEN 100
#define NUM_COMMANDS 9

#define SUCCESS 1
#define FAIL 0

void init_globals();

// parsing, tokenizing
char * read_line();
int tokenize(char * str, char ** tokens);
int exec_tokens(char ** tokens);
int reset_tokens(char ** tokens);

// argument management for running programs
int make_args(char ** tokens, int index);
int get_args_end(char ** tokens, int index);
int reset_args();
int free_args();

// various commands
int move_to_dir(char * directory);
void where_am_i();
int start_program(char ** tokens, int i);
int run_program(char ** tokens, int i, int back);
void repeat(char ** tokens, int i, int num_repeats);

// cleanup functions (frees)
void ext_PID();
void kill_zombies(int print_it);
int all_cleanup(char ** tokens);

typedef struct array_list {
 char ** arr; // array of names
 int size, cap;
} array_list;

// Array Lists help to deal with global history arrayList
// History does not have a limit
void print_history();
array_list * makeArrayList(int capacity);
int add_to_history(char * nameToAdd);
int expandArrayList();
int freeArrayList();
int clear_history();

void print_help();

typedef struct
{
  array_list * history;
  int quit;
  char * cur_dir;
  int * pids;
  int pid_point;
  char * args[TOKENS_SIZE+1];
  int arg_counter;
} GLOBALS;

GLOBALS global;

static char * input_line = (char *)NULL;

// not used, but could be used for a help directory in the future
char * commands[NUM_COMMANDS][2] = {
  { "cd", "\tChange to directory DIR" },
  { "whereami", "Print current directory" },
  { "history [-c]", "Display history or clear history if -c" },
  { "quit", "\tQuit the shell" },
  { "start program [params]", "start a program with certain parameters" },
  { "background program [params]",
  "Immediately prints out PID of program and return prompt"},
  { "exterminate [params]",
  "Terminate program with specific PID."},
  { "exterminateall",
  "kills all processes started during shell use."},
  { "repeat n [params]",
  "repeat the given program n number of times in the background. "},
};

// Turtle greets every shell user
int hello()
{  printf("_____________________________________________________________\n");
  printf("A turtle doesn't have a shell_██████████████__________________\n");
  printf("__________________________████▓▓▓▓▓▓▒▒▒▒▓▓▓▓████______________\n");
  printf("____████████____________██▒▒▓▓▓▓▓▓▓▓▒▒▒▒▓▓▓▓▓▓▓▓██____________\n");
  printf("__██░░░░██░░██________██▒▒▒▒▒▒▓▓▓▓▒▒▒▒▒▒▒▒▓▓▓▓▒▒▒▒██__________\n");
  printf("__██░░░░░░░░░░██____██▒▒▓▓▓▓▒▒▒▒▒▒▒▒▓▓▓▓▒▒▒▒▒▒▒▒▓▓▓▓██________\n");
  printf("__██░░░░▒▒░░░░██____██▓▓▓▓▓▓▓▓▒▒▒▒▓▓▓▓▓▓▓▓▒▒▒▒▓▓▓▓▓▓██________\n");
  printf("__██▒▒▒▒░░░░░░██__██▒▒▓▓▓▓▓▓▓▓▒▒▒▒▓▓▓▓▓▓▓▓▒▒▒▒▓▓▓▓▓▓▓▓██______\n");
  printf("__████████░░░░██__██▒▒▒▒▓▓▓▓▒▒▒▒▒▒▒▒▓▓▓▓▒▒▒▒▒▒▒▒▓▓▓▓▒▒██______\n");
  printf("________██░░░░██__██▓▓▒▒▒▒▒▒▒▒▓▓▓▓▒▒▒▒▒▒▒▒▓▓▓▓▒▒▒▒▒▒▒▒██______\n");
  printf("________██░░░░░░████▓▓▓▓▒▒▒▒▓▓▓▓▓▓▓▓▒▒▒▒▓▓▓▓▓▓▓▓▒▒▒▒▓▓██______\n");
  printf("__________██░░░░░░██▓▓▓▓▒▒▒▒▓▓▓▓▓▓▓▓▒▒▒▒▓▓▓▓▓▓▓▓▒▒▒▒▓▓████████\n");
  printf("__________██░░░░░░██▓▓████████████▒▒▒▒▒▒▒▒██████████████░░░░██\n");
  printf("____________██░░░░░░██▒▒▒▒▒▒▒▒▒▒▒▒████████▒▒▒▒▒▒▒▒▒▒▒▒██░░██__\n");
  printf("______________████████▒▒████████▒▒▒▒▒▒▒▒▒▒▒▒████████▒▒████____\n");
  printf("______________________████░░░░████████████████░░░░████________\n");
  printf("__A turtle is it's______██░░░░██____________██░░░░██need_help?\n");
  printf("_________Shell__________██░░░░██____________██░░░░██__type____\n");
  printf("________________________██▓▓░░██____________██▓▓░░██____help__\n");
  printf("________________________████████____________████████__________\n");
  return 1;
}


// main function loop.
// while we haven't seen 'quit' continue looping
int main(argc, argv) int argc; char **argv;
{
  hello();
  init_globals();

  char ** tokens;
  tokens = (char **) calloc(TOKENS_SIZE, sizeof(char *));

  // I/O loop.
  // Fill input_line pointer with user input, tokenize it, run commands with it.
  while(!global.quit)
  {
    // check if input line exists, if so, free it and set to null.
    if (input_line)
    {
      free(input_line);
      input_line = (char *)NULL;
    }
    if (tokens)
    {
      if (!reset_tokens(tokens))
      {
        printf("Oops! error resetting tokens. \n");
      }
    }

    // Get a line from the user.
    // now using updated self-implemented read-line function()
    input_line = read_line();

    // If the line has any text in it, save it on the history.
    if (input_line && *input_line && strlen(input_line) > 0)
      add_to_history(input_line);

    // make line into tokens
    tokenize(input_line, tokens);
    // execute those tokens
    exec_tokens(tokens);
  }

  all_cleanup(tokens);

  printf("===========================================\n");
  printf("Goodbye....Turtle Crawls into its-Shellf\n");
  printf("===========================================\n");

  return 0;
}


// initialize globals needed for execution
void init_globals()
{
  char buff[FILE_MAX];
  getcwd(buff, FILE_MAX);
  global.history = makeArrayList(20);
  global.cur_dir = buff;
  global.quit = 0;
  global.pids = (int *) calloc(1, sizeof(int)*100);
  global.pid_point = 0;
  int i;
  for(i = 0; i < TOKENS_SIZE+1; i++)
  {
    global.args[i] = (char *)NULL;
  }
  global.arg_counter = 0;
}


// free tokens and globals that we must free
int all_cleanup(char ** tokens)
{  int i;
  for(i = 0; i < TOKENS_SIZE; i++)
  {
    free(tokens[i]);
    tokens[i] = NULL;
  }
  kill_zombies(0);
  freeArrayList(global.history);
  free(global.history);
  free(input_line);
  free(global.pids);
  free_args();
  free(tokens);


  return 1;
}


// Parser function
// takes in tokens array of strings
// returns 1 if successfully parsed
// Long "If and else if" string to do parsing
int exec_tokens(char ** tokens)
{
  int i = 0;

  if(tokens[i] == NULL)
    return SUCCESS;
  else
  {
    if (DEBUG)
    {
      printf("debug: token %d: %s\n", i, tokens[i]);
    }
    if (strcmp(tokens[i], "help") == 0)
    {
      print_help();
    }
    else if (strcmp(tokens[i], "quit") == 0)
    {
      global.quit = 1;
    }
    else if (strcmp(tokens[i], "cd") == 0)
    {
       i += 1;
       if (tokens[i] == NULL)
       {
         printf("! Oops1! must specify directory to which we will move\n");
         return FAIL;
       }
     else if (move_to_dir(tokens[i]) == 0)
     {
        printf("! Oops2! Error moving to the directory\n");
        return FAIL;
      }
    }
    else if (strcmp(tokens[i], "whereami") == 0)
    {
      where_am_i();
    }
    else if (strcmp(tokens[i], "history") == 0)
    {
      i+=1;
      if (tokens[i] == NULL)
      {
        print_history();
        // display history
      }
      else
      {
        if (strcmp(tokens[i], "-c") == 0)
        {
          clear_history();
        }
      }
    }
    else if (strcmp(tokens[i], "start") == 0)
    {
      if (tokens[++i] == NULL || strcmp(tokens[i], "program") != 0)
      {
        printf("! Oops! you must type 'start program'\n");
      }
      else if (strcmp(tokens[i++], "program") == 0)
      {
        run_program(tokens,i, 0);
      }
    }
    else if (strcmp(tokens[i], "background") == 0)
    {
      if (tokens[++i] == NULL || strcmp(tokens[i], "program") != 0)
      {
        printf("! Oops! you must type 'start program'\n");
      }
      else if (strcmp(tokens[i++], "program") == 0)
      {
        run_program(tokens,i, 1);
      }
    }
    else if (strcmp(tokens[i], "exterminate") == 0)
    {
      if (tokens[++i] == NULL)
      {
        printf("! Oops! you must include PID to kill\n");
      }
      else
      {
        int pid_int = atoi(tokens[i]);
        ext_PID((int) pid_int);
      }
    }
    else if (strcmp(tokens[i], "exterminateall") == 0)
    {
      kill_zombies(1);
    }
    else if (strcmp(tokens[i], "repeat") == 0)
    {
      if (tokens[++i] == NULL)
      {
        printf("! Oops! you must include num_repeats\n");
      }
      else
      {
        int repeats = atoi(tokens[i++]);
        repeat(tokens, i, repeats);
      }
    }
  }
  // reset_args();

  return SUCCESS;
}


// given a character array from readline, tokenize into an array of char arrays
int tokenize(char * str, char ** tokens)
{
  // Declaration of delimiter
   char s[4] = " ";
   char * tok;
   int len;
   int token_idx;
   // Use of strtok
   // get first token
   tok = strtok(str, s);

   token_idx = 0;
   // Checks for delimeter
   while (tok != NULL && token_idx < TOKENS_SIZE)
   {
       len = strlen(tok);
        tokens[token_idx] = (char *) calloc(len + 1, sizeof(char));

       strcpy(tokens[token_idx++], tok);
       // Use of strtok
       // go through other tokens
       tok = strtok(0, s);
   }
   while (token_idx < TOKENS_SIZE)
   {
     tokens[token_idx++] = NULL;
   }
   return 1;
}

// get current working directory and print
void where_am_i()
{
  char buff[FILE_MAX];
  getcwd(buff, FILE_MAX);
  global.cur_dir = buff;
  printf("$ %s\n", global.cur_dir);
}


// cd command
int move_to_dir(char * directory)
{
  if (chdir(directory) != 0)
  {
    return FAIL;
  }

  char buff[FILE_MAX];
  getcwd(buff, FILE_MAX);

  global.cur_dir = buff;
  return SUCCESS;
}


// prints history
void print_history()
{
      int i;
     printf("\nHistory\n=================\n");
     for (i = 0; i < global.history->size; i++)
     {
        printf(" %s\n", global.history->arr[i]);
      }
      printf("\n");
}


// Modified from GNU docs
// history -c command
int clear_history()
{
  if (!freeArrayList())
  {
    printf("! Oops! Trouble freeing the history list");
    return FAIL;
  }
  free(global.history);
  global.history = (array_list *) NULL;
  global.history = makeArrayList(20);

  return SUCCESS;
}


// feed it tokens, and the index where args start at tokens
// helps us know how much to iterate to reach all arguments
int get_args_end(char ** tokens, int i)
{
  while (i < TOKENS_SIZE)
  {
    if (tokens[i] == NULL)
      return i - 1;
    i++;
  }
  return i - 1;
}

// add all arguments to global arguments array
int make_args(char ** tokens, int first)
{
  int start = first;
  int end = get_args_end(tokens, first);

  int i = 0;
  while(start <= end)
  {
    global.args[i] = (char *) realloc(global.args[i], (strlen(tokens[start])+1)*sizeof(char));
    strcpy(global.args[i], tokens[start++]);
    if (DEBUG) printf("argument %d : %s\n", i, global.args[i]);
    i++;
  }
  global.arg_counter = i;
  global.args[i] = (char *) NULL;


  return end;
}


// free memory of all tokens
int reset_tokens(char ** tokens)
{
  int i;

  for(i = 0; i < TOKENS_SIZE; i++)
    free(tokens[i]);

    return SUCCESS;
}

// free memory of all tokens
int reset_args()
{
  int i;
  for(i = 0; i < TOKENS_SIZE+1; i++)
  {
    free(global.args[i]);
    global.args[i] = (char*)NULL;

  }

  global.arg_counter = 0;
  return SUCCESS;
}

// free memory of args for end of program all_cleanup()
int free_args()
{
  int i;
  for(i = 0; i < TOKENS_SIZE+1; i++)
    free(global.args[i]);

  return SUCCESS;
}


// program for background or start program
// repeat is done in another function
int run_program(char ** tokens, int i, int background)
{
  int child_status;
  if (tokens[i] == NULL)
  {
    printf("! Oops! Must include file to run\n");
    return 0;
  }

  char * file_name = (char *) calloc(strlen(tokens[i])+1, sizeof(char));
  strcpy(file_name, tokens[i]);

  if (DEBUG) printf("FILE NAME: %s\n", file_name);

  int end = make_args(tokens,  i);
  pid_t parent = getpid();
  pid_t pid = fork();

  // print all arguments
  if (DEBUG)
  {
    int j = 0;
    while(j < global.arg_counter)
    {
      printf("%s\n", global.args[j++]);
    }
  }

  if (pid == -1)
  {
    printf("! Oops! Fork Failed\n");
  }
  else if (pid == 0)
  {
    // we are doing background, we must setsid()
    // this is the child
    if (background)
    {
      setsid();
    }
    if (execv(file_name, global.args) == -1)
    {
      printf("! Oops! Couldn't find the program\n");
      exit(pid);
    }
  }
  else
  {
    // parent, print PID of child, wait for child to finish (maybe)
    int status;
    if (background)
    {
      printf("* pid of child: %d\n", pid);
      global.pids[global.pid_point++] = pid;
      waitpid(-1, &status, WNOHANG);
    }
    else
    {
      waitpid(pid, &status, 0);
    }
  }

  free(file_name);
  return SUCCESS;
}

// exterminate a single program by PID
void ext_PID(int pid)
{
  int kill_return = kill(pid, SIGKILL);
  if(kill_return){
      int status;
      printf("! Oops! Unable to kill child process.\n");
      waitpid(pid, &status, 0);
  } else {
      printf("* PID %d killed.\n", pid);
  }
}


// repeat n program [-params]
// repeat a program with parameters n number of times.
// leverage while loop and fork/exec calls
void repeat(char ** tokens, int i, int num_repeats)
{
  int back = 1;
  int child_status;
  if (tokens[i] == NULL)
  {
    printf("! Oops! Must include file to run\n");
    return;
  }

  char * file_name = (char *) calloc(strlen(tokens[i])+1, sizeof(char));
  strcpy(file_name, tokens[i]);

  if (DEBUG) printf("FILE NAME: %s\n", file_name);
  int end = make_args(tokens,  i);
  int rep = 0;

  printf("* PIDs:");
  while(rep < num_repeats)
  {
    pid_t parent = getpid();
    pid_t pid = fork();

    if (pid == -1)
    {
      printf("Oops!\n");
    }
    else if (pid == 0)
    {
      // child
      if (back)
        setsid();
      if (execv(file_name, global.args) == -1)
        exit(pid);
    }
    else
    {
      // parent
      int status;
      if (back)
      {
        printf(" %d", pid);
        global.pids[global.pid_point++] = pid;
        waitpid(-1, &status, WNOHANG);
      }
      else
      {
        waitpid(pid, &status, 0);
      }
    }
    rep++;
  }
  printf("\n");
  free(file_name);
  return;
}


// EXTERMINATEALL extra credit
// iterate through zombies , kill them, reset global point
void kill_zombies(int print_it)
{
  int cp;
  if (print_it == 1)
    printf("* Murdering %d processes: ", global.pid_point);
  for (int i = 0; i < global.pid_point; i++)
  {
    if (global.pids[i] != 0)
    {
      cp = kill(global.pids[i], SIGKILL);
      if (cp)
      {
        int status;
        waitpid(global.pids[i], &status, 0);
      }
      else
      {
        printf("%d ", global.pids[i] );
      }
    }
    global.pids[i] = 0;
  }
  printf("\n");
  global.pid_point = 0;
}


// Returns character array that is the line.
// tokenizer will tokenize later. ]]]]]]
// prints the pre-line hash # symbol
char * read_line()
{
    printf("# ");

    int buff_n = 1024;
    char * input = malloc(buff_n * sizeof(char));

    if (input == NULL)
    {
      printf("Error: read_line() malloc Failed");
      return NULL;
    }
    char c;
    int buff_i = 0;

    // loop through chars of input until newline of EOF (ctl-d)
    while((c = getchar()) != '\n')
    {
      // if user is crtl+d , then we don't return any strings
      if (c == EOF)
      {
        free(input);
        return NULL;
      }
      // if we exceed the max size, realloc
      if (buff_i >= buff_n-1)
      {
        buff_n *= 2;
        input = realloc(input, buff_n);
      }
      input[buff_i++] = c;
    }
    // add null terminatoin before return
    input[buff_i] = '\0';

    return input;
}


// constructor function to calloc() space for arrayList
// preconditions: function takes in current size and capacity of array_list
//                NOTE: current size does not reflect the first name in the arrayList,
//                      which acts as a row label.
// postconditions: return a pointer to an initialized array_list pointing to given input
array_list * makeArrayList(int capacity)
{
  array_list * aL = (array_list *)calloc(1, sizeof(array_list));

  if (aL != NULL)
  {
    // initialize values
    aL->arr = (char **)calloc(capacity, sizeof(char *));
    aL->size = 0;
    aL->cap = capacity;

    // check if calloc functions correctly
    if (aL->arr == NULL)
    {
      free(aL);
      aL = NULL;
    }
  }
  // returns successfully created list, or null if unsuccessful
  return aL;

}


// preconditions: aL is initialized
// postconditions: aL capacity increases by 1, size and other parameters unchanged.
int expandArrayList()
{
  int updatedCap = global.history->cap * 2;
  char ** newNames = (char **) realloc(global.history, updatedCap * sizeof(char *));

  //check if it completed
  if (newNames == NULL)
    return FAIL;

  global.history->cap = updatedCap;
  global.history->arr = newNames;

  return SUCCESS;
}


// update arrayList by adding nameToAdd
// we should only be adding to an already initialized arrayList of at least size 1
int add_to_history(char * nameToAdd) {

  //arrayList full in this case
  if (global.history->size == global.history->cap)
    if (expandArrayList(global.history) == FAIL)
      return FAIL;

  global.history->arr[global.history->size] = realloc(
                                    global.history->arr[global.history->size],
                                    (strlen(nameToAdd)+1)*sizeof(char));

  strcpy(global.history->arr[global.history->size], nameToAdd);
  global.history->size++;

  return SUCCESS;
}


// free array list of strings.
int freeArrayList()
{
  int i;
  for(i = 0; i < global.history->cap; i++)
  {
    free(global.history->arr[i]);
    global.history->arr[i] = (char *) NULL;
  }

  free(global.history->arr);

  return SUCCESS;

}


// print the help commands.
void print_help()
{
  int i;

  printf("\nCommands\t\t\t\tActions\n");
  printf("\n==========================================================================\n");
  printf("\n");

  for (i = 0; i < NUM_COMMANDS; i++)
  {
    if ( i < 4)
      printf("%s\t\t\t\t%s\n", commands[i][0], commands[i][1]);
    else if (i < 5)
      printf("%s\t\t\t%s\n", commands[i][0], commands[i][1]);
    else if ( i == 6 || i == 8)
      printf("%s\t\t\t%s\n", commands[i][0], commands[i][1]);

    else if (i == 7 || i == 9)
      printf("%s\t\t\t\t%s\n", commands[i][0], commands[i][1]);
    else
      printf("%s\t\t%s\n", commands[i][0], commands[i][1]);
  }
  printf("\n");
}
