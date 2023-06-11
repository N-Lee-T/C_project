#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <stddef.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdint.h>


/* Global variables
 *
 *
 *
 *
 */

// Global variable to hold status of child process 
pid_t kidstat;

// Variable to hold exit status of foreground child process - "$?"
int fgkidstat;

// Variable to hold the PID of the last background process - "$!"
pid_t bgproc;

// Variable to hold status of last background process to exit
int bgkidstat = 0;

// an array of pids of bg processes - this must be checked at the beginning of the loop
//pid_t bgpids[100]; // UNUSED - SHOULD BE AN ARR OF POINTERS

// track the place of the index so we can place child bg pids in the array
int bgpidsIndex; // UNUSED - 

// Should the new process be run in the background? To set child PID as new BG proc in exec func
int bgnew = 0; 

// char to pass to string replacement function
char *str_gsub(char *restrict *restrict haystack, char const *restrict n1, char const *restrict sub);

// holds the number of words tokenized
int count;

// Variable pointer to hold value of input redirection 
char *newin = NULL;

// Variable pointer to hold value of output redirection
char *newout = NULL;

// exit status of last foreground comm
int toexit = 0;

// Exit status if not fgkidstat
int extst = 0;

// hold the default signal handling when smallsh invoked
struct sigaction oldacts;

/* String replacement function
 * Taken directly from instructor video 
 *
 *
 *
 */
 char *str_gsub(char *restrict *restrict haystack, char const *restrict n1, char const *restrict sub)
  
  {
      char *str = *haystack;
      size_t haystack_len = strlen(str);
      size_t const needle_len = strlen(n1),
                 sub_len = strlen(sub);
    
      // need to research how strstr works??

      for (; (str = strstr(str, n1));) {
        ptrdiff_t off = str - *haystack;
        if (sub_len > needle_len) {
            str = realloc(*haystack, sizeof **haystack * (haystack_len + sub_len - needle_len + 1));
            if (!str) goto exit;
            str = *haystack + off;
        }
        memmove(str + sub_len, str + needle_len, haystack_len + 1 - off - needle_len);
        memcpy(str, sub, sub_len);
        haystack_len = haystack_len + sub_len - needle_len;
        str += sub_len;
      }
      str = *haystack;
      
      if (sub_len < needle_len) {
        str = realloc(*haystack, sizeof **haystack * (haystack_len + 1));
        if (!str) goto exit;
        *haystack = str;
      }
      exit:
          return str;
    }


/* Execution 
 *
 *
 *
 *
 */
int execution(char *wordarr[]){

  if (strcmp(wordarr[0], "exit") == 0) {
    // set exit status variable here
    toexit = 1;
    if (count > 2) {
      goto exiterr;
    }
    if (wordarr[1]) {
      // check validity: is it a digit?
      if (isdigit(*wordarr[1]) != 0){
        extst = atoi(wordarr[1]);
      } else {
        fprintf(stderr, "Non-digit argument to exit()\n");
      }
    } else {
       extst = fgkidstat;
    }
    goto end;
 } 
  if (strcmp(wordarr[0], "cd") == 0) goto change;

  // check: change stdin or stdout
  if (newin != NULL) { 
  } 
  int newproc = fork();
  if (bgnew == 1) bgproc = newproc;
  switch(newproc){
    case -1:
      perror("whoa! fork() didn\'t:");
      break;
    case 0:
      // change back to default signal handling
       sigaction(SIGINT, &oldacts, NULL);
       sigaction(SIGTSTP, &oldacts, NULL);
      // check: change stdin 
      if (newin != NULL) { 
        int newfdin = open(newin, O_RDONLY);
        if (newfdin == -1){
          perror("err opening input file: ");
          close(newfdin);
          free(newin);
          newin = NULL;
          goto exiterr;
        }
        
        // copy the fd stream to newfdin
        int newstreamin = dup2(newfdin, 0);
        if (newstreamin == -1){
          perror("dup2 error: ");
          close(newfdin);
          free(newin);
          newin = NULL;
          goto exiterr;
        }
        close(newfdin);
      }
      
      // check: change stdout
      if (newout != NULL) { 
        int newfdout = open(newout, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0777);
        if (newfdout == -1){
          perror("err opening output file: ");
          close(newfdout);
          free(newout);
          newout = NULL;
          goto exiterr;
        }
        // copy the fd stream to newfdout
        int newstreamout = dup2(newfdout, 1);      
        if (newstreamout == -1){
          perror("dup2 error: ");
          close(newfdout);
          free(newout);
          count -= 1;
          goto exiterr;
        }
        close(newfdout); 
      }

      // execute the code!
      if ((wordarr[0]) && (wordarr)) execvp(wordarr[0], wordarr); 
      // nothing here will happen - unless it's an error!
      goto exiterr;

    default:
    // background process: no need to wait 
       if (bgnew == 1) {
         break;
         // otherwise, run in foreground; need to wait
         } else {
        int newex; 
        newproc = waitpid(newproc, &newex, 0);
         if WIFEXITED(newex) {
           fgkidstat = WEXITSTATUS(newex);
         } else {
           fgkidstat = WTERMSIG(newex) + 128;
           // if fgkidstat is stopped
           if (fgkidstat == SIGTSTP) kill(newproc, SIGCONT);
           bgproc = newproc;
         }
    }
  }
  goto end;

change:
  if (!wordarr[1]) {
    int changedir = chdir(getenv("HOME")); 
    if (changedir < 0) goto exiterr; 
  } else {
    int changedir = chdir(wordarr[1]);
    if (changedir < 0) goto exiterr;
  }
  goto end;
  
end: 
 bgnew = 0;
 for (int i = 0; i < count + 1; ++i){  
   free(wordarr[i]);
   wordarr[i] = NULL;
  }

 return 0;

exiterr:
    
    perror("\nexit error ");
//     printf("bgproc is %jd, fgkidstat is %i, wordarr[0] is %s, wordarr[1] is %s, wordarr[2] is %s\n", (intmax_t) bgproc, fgkidstat, wordarr[0], wordarr[1], wordarr[2]);
//     exit(1);
    toexit = 1;
    return 1;
}


/* Parsing
 *
 *
 *
 *
 * */
int pars(char *wordarr[]){
  if (!wordarr[0]) goto parsFail;
  for (int i = 0; i < count; i++){
    if (strlen(wordarr[i]) == 1){
      if (strcmp(wordarr[i], "#") == 0) {
        wordarr[i] = NULL;
        break;
      }
      if ((strcmp(&wordarr[i][0], "&") == 0) && ((count-1) == i)){
        bgnew = 1;
        wordarr[i] = NULL;
      }
    } 
      if (strcmp(&wordarr[i-1][0], "<") == 0) {
        newin = malloc(sizeof(char) * strlen(wordarr[i])+1);
        if (newin == NULL) {
          perror("newin malloc error ");
          goto parsFail;
        } else {
        newin = strdup(wordarr[i]);
        wordarr[i-1] = NULL;
        }
      } else if (strcmp(&wordarr[i-1][0], ">") == 0) {
        newout = malloc(sizeof(char) * strlen(wordarr[i])+1);
        if (newout == NULL) {
          perror("newout malloc error ");
          goto parsFail;
        } else {
        newout = strdup(wordarr[i]);
        wordarr[i-1] = NULL;
        }
    }
  }
return 0;

parsFail:
  return 1;

 }


/* Tokenize
 *
 *
 *
 *
 */
int tokenize(char *lnptr, char *wordarr[]){
  
  count = 1;

  // save a pointer for strtok_r
  char *savptr; // core dump when it's set to null??? = NULL;

  // get delimiter or use default
  char *delim = getenv("IFS");
  if (!delim) delim = " \t\n";

  // char to hold value of token temporarily
  char *new = NULL;

  // first element is the command - necessary for execvp() in execute function
  new = strtok_r(lnptr, delim, &savptr);
  if (new == NULL) goto end;

  wordarr[0] = strdup(new);
  new = NULL; 
    for (;;){
      new = strtok_r(NULL, delim, &savptr);
      if ((new == NULL) || (strcmp(new, "\n") == 0)) goto end;
      wordarr[count] = strdup(new); // ERR
      if (strncmp(wordarr[count], "~/", 2) == 0) {
        str_gsub(&wordarr[count], "~", getenv("HOME")); 
      }
      count++;
    }
end:
    wordarr[count] = NULL;
//     free(savptr);
    free(new);
    new = NULL;
    return 0;
}



/* Expansion
 *
 *
 *
 *
 * */
int expand(char *lnptr){

  // "$$" --> smallsh PID
  // get the pid and save it to a string 
  pid_t currpid = getpid();
  char *pidchar = malloc(sizeof(pid_t)*sizeof(char));
  if (asprintf(&pidchar, "%i", currpid) == -1) {
    perror("asprintf failure at pid expansion ");
    goto failure; 
  }
  str_gsub(&lnptr, "$$", pidchar); 
  free(pidchar);

  // "$?" --> exit status of last foreground process
  // get the exit status 
  char *status = malloc(sizeof(int)*(sizeof(char)));
  if (asprintf(&status, "%i", fgkidstat) == -1) {     // CHANGED btwn FGKIDSTAT & extst
    perror("asprintf failure at fg proc expansion ");
    goto failure; 
  }
  str_gsub(&lnptr, "$?", status);
  free(status);

  // "$!" --> most recent background process PID
  // get the pid and save it to a string 
  char *bg = malloc(sizeof(int)*sizeof(char));
  if (!bgproc) str_gsub(&lnptr, "$!", "");        /// CHANGED!!
  if (asprintf(&bg, "%i", bgproc) == -1) {
    perror("asprintf failure at bg pid  expansion ");
    goto failure; 
  }
  str_gsub(&lnptr, "$!", bg);
  free(bg);
  return 0; 

failure:
  fprintf(stderr, "expansion failed\n");
  return 1;
}



/* Status check func
 *
 *
 *
 *
 */
void statuscheck(void){
  errno = 0;
  // then, enter a loop where we check for kids
checkloop:
  kidstat = waitpid(0, &bgkidstat, WUNTRACED | WNOHANG | WSTOPPED | WCONTINUED); 
  if (kidstat <= 0) { 
    goto end;  
   } else {
   if (WIFEXITED(bgkidstat)){
      fprintf(stderr, "Child process %jd done. Exit status %d.\n", (intmax_t) kidstat, WEXITSTATUS(bgkidstat));      
      goto end;                                                                                          
   } else if (WIFSTOPPED(bgkidstat)) {
       kill(kidstat, SIGCONT); 
       fprintf(stderr, "Child process %jd stopped. Continuing\n", (intmax_t) kidstat);
//        goto checkloop;
   } else if (WIFSIGNALED(kidstat)) {
      if (WTERMSIG(bgkidstat) == 127) {
        if (kill(kidstat, SIGCONT) == 0){
        } else {
          perror("Cannot continue child process: ");
          errno = 0;
        }
   } else if (WIFCONTINUED(kidstat)) {
//        goto checkloop;
   } else {
      fprintf(stderr, "Child process %jd done. Signaled %d.\n", (intmax_t) kidstat, WTERMSIG(bgkidstat));
      fgkidstat = WTERMSIG(bgkidstat) + 128;
     }
   } 
   goto checkloop;
 }
//   goto checkloop;
  
 
end:
  if ((errno != 0) && (errno != ECHILD)) perror("check function Error!: ");
  errno = 0;
  kidstat = 0;
  return;
}


// signal handler for sigint that does nothing
 
void handle_else(){
}

int main(void){

  char *lnptr = NULL;
  size_t len = 0;
  ssize_t nread = 0;
  char *ps1 = NULL;

  // create sigaction struct 
  struct sigaction sigint_dont = {0};
  sigint_dont.sa_handler = handle_else;
  sigint_dont.sa_flags = 0;

  // create sigaction struct 
  struct sigaction sigint_do = {0};
  sigint_do.sa_handler = SIG_IGN;
  sigfillset(&sigint_do.sa_mask);
  sigint_do.sa_flags = 0;

   // assign struct to handle sigtstp and sigint
  sigaction(SIGINT, &sigint_do, NULL); //&oldacts);
  sigaction(SIGTSTP, &sigint_do, NULL); // &oldacts);

getinput:
  ps1 = getenv("PS1");
  if (ps1 == NULL) ps1 = "";

  // check the status of background child processes
  statuscheck(); 
  fprintf(stderr, "%s", ps1);
  if (lnptr != NULL) lnptr = NULL;
  // set the signal handler for sigint while getline(), then change back
  // getline: copied example from Linux man pages
  int new = sigaction(SIGINT, &sigint_dont, NULL);
  if (new < 0) {
    perror("sigaction error");
    clearerr(stdin);
  }
  nread = getline(&lnptr, &len, stdin);
  // check for errors with getline()
  if (nread < 0) {
    if (feof(stdin)) goto exit;
    if (errno == EINTR) {
     lnptr = NULL;
     len = 0;
     nread = 0;
     clearerr(stdin);
     goto getinput;
    } else if (errno == ENOMEM) {
      goto exit;
     }  
   }

   if ((strcmp(lnptr," \n") == 0) || (strcmp(lnptr,"\t\n") == 0)) {
     len = 0;
     nread = 0;    
     lnptr = NULL;
     goto getinput;
    }
  
  sigaction(SIGINT, &sigint_do, NULL); // &sigint_dont);
  len = 0;
  nread = 0;

  // call the expansion function
  if (expand(lnptr) > 0){
    perror("expansion Error!: ");
    goto failureproc;
  }

  // call the tokenize function
  char *wordarr[512];
  tokenize(lnptr, wordarr);
  
  // pass the result of tokenize to parsing function
  if (pars(wordarr) == 1) goto failureproc;

  // execute the statement
  if (execution(wordarr) == 1) {
    fprintf(stderr, "\nexecution error\n");
    goto failureproc;
  }
  if (toexit == 1) goto exit;
  if (newout != NULL){
    free(newout);
    newout = NULL;
  }
  if (newin != NULL){
    free(newin);
    newin = NULL;
  }
  lnptr = NULL;
  goto getinput;

failureproc:
  if (wordarr[0] != NULL) {
  for (int i = 0; i < count ; ++i){ 
    free(wordarr[i]);
    wordarr[i] = NULL;
   }
  }
  goto getinput;

exit:
  fprintf(stderr, "\nexit\n");
  if (toexit == 0) exit(fgkidstat);

  // kill all child processes in same group
  kill(0, SIGINT);
  exit(extst);
}

