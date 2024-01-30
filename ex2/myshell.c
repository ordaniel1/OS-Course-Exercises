#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>



int prepare(void){
    __sighandler_t sig;
    sig=signal(SIGINT,SIG_IGN); //  shell shouldn't terminate upon SIGINT
    if(sig==SIG_ERR) { //there is an error with signal
        perror("signal error");
        return 1;
    }
    return 0; //success
}







/*
 * This method check the error's type and returns:
 * 0    if errno equals to ECHILD or EINTER
 * -1   otherwise.
 */
int error_handler(){
    if (errno==ECHILD || errno==EINTR){
        errno=0;
        return 0; //ignore error
    }
    else{
        return -1; //there is an error, we can't ignore it.
    }
}









/* This method executing commands (with no 'pipe').
 * arguments:
 *
 * char **arglist
 * int bgd:
 *      if bgd=1, execute a command in the background
 *      if bgd=0, execute a command as foreground command
 *
 * int redirect:
 *      if redirect=1 - standard output will be redirected to an outputfile
 *
 * int fileIndex:
 *      if  redirect=1 - arglist[fileIndex] = output file name
 *      else, fileIndex is meaningless.
 *
 *
 */
int executing_commands(char **arglist ,int bgd, int redirect, int fileIndex){
    pid_t cpid; //pid for one child process
    int fd, dp, ex; // will save the values that will be returned by open (redirection case), dup2 and execvp
    fd=0; //initialize to 0  in order to avoid a warning by using this variable in a child process (because of the conditions)


    //according to the instructions, we need to open the specified file before running child process
    if (redirect==1) { // there is an output redirecting
        fd = open(arglist[fileIndex], O_WRONLY | O_CREAT, 0777); //open file, or create it.
        if (fd == -1) { //there is an error with open
            perror("open error");
            return 1; //that's not a success but according to instructions and the forum - we can't terminate "the shell".
        }
    }

    cpid=fork(); //fork -  for a child process
    if(cpid==-1){ //there is an error with fork
        perror("fork error");
        return 0;
    }


    //child process
    if (cpid==0){
        if(bgd==0){ //if child process is not a background process
            __sighandler_t sig;
            sig=signal(SIGINT, SIG_DFL);  // foreground child process should terminate upon SIGINT
            if (sig==SIG_ERR){ //there is an error with signal
                perror("signal error");
                exit(1);
            }
        }

        if (redirect==1) { // there is an output redirecting
            dp=dup2(fd,STDOUT_FILENO); //now STDOUT_FILENO refers to fd
            if (dp<0){ //there is an error with dup2
                perror("dup error");
                exit(1);
            }
            close(fd); //close fd before execvp
        }

        ex=execvp(arglist[0], arglist); //replace the current process image with a new image - of the command.
        if(ex==-1){ //there is an error with execvp
            perror("execvp error");
            exit(1);
        }
    }


    //parent - shell
    if (redirect==1) { //close fd if needed (in case of redirection output)
        close(fd);
    }

    if(bgd==0){ //command (child process) isn't executed in background
        // in order to avoid a zombie - the shell will wait for the command's completion. pass child's pid to waitpid.
        if(waitpid(cpid,NULL,0)==-1){ // if there is an error with waitpid
            if(error_handler()==-1){ //if there is a "real" error (errno!=ECHILD, EINTR)
                perror("waitpid error");
                return 0;
            }
        }
    }

    else{ //command is executed in background
        __sighandler_t sig;
        sig=signal(SIGCHLD, SIG_IGN); //avoid background process from being a zombie
        if (sig==SIG_ERR){ //there is an error with signal
            perror("signal error");
            return 0;
        }
    }
    return 1; //success

}









/* This method implements single-piping.
 * arguments:
 *
 * char **arglist
 * int i s.t arglist[i]='|'
 *
 * returns:
 * 1   if succeed
 * 0   otherwise
 */
int single_piping(char **arglist, int i){
    pid_t cpid, c2pid; // 2 childs for 2 processes
    int dp,ex; // will save the values that will be returned by dup2 and execvp
    int pipefd[2]; //the array for pipe
    if (pipe(pipefd)==-1){ // there is an error with pipe
        perror("pipe error");
        return 0;
    }

    cpid=fork(); //first fork for first child process
    if(cpid==-1){ // there is an error with fork
        perror("fork error");
        return 0;
    }


    //first child
    if (cpid==0){

        __sighandler_t sig;
        sig=signal(SIGINT, SIG_DFL); // foreground child process should terminate upon SIGINT
        if(sig==SIG_ERR){ //there is an error with signal
            perror("signal error");
            exit(1);
        }

        close(pipefd[0]); //no read
        dp=dup2(pipefd[1],STDOUT_FILENO); // STDOUT_FILENO now refers to pipefd[1]
        if(dp<0){ //there is an error with dup2
            perror("dup error");
            exit(1);
        }

        close(pipefd[1]); // close "write" before execvp !
        arglist[i]=NULL; //we want to pass to execvp only the first command (before '|')
        ex=execvp(arglist[0],arglist); //replace the current process image with a new image - of the command.
        if(ex==-1){ //there is an error with execvp
            perror("execvp error");
            exit(1);
        }

    }

    //parent - shell
    c2pid=fork(); //second fork for second child process
    if(c2pid==-1){ // there is an error with fork
        perror("fork error");
        return 0;
    }



    //second child
    if(c2pid==0){

        __sighandler_t sig;
        sig=signal(SIGINT, SIG_DFL); // foreground child process should terminate upon SIGINT
        if(sig==SIG_ERR){ //there is an error with signal
            perror("signal error");
            exit(1);
        }

        close(pipefd[1]); // no "write"
        dp=dup2(pipefd[0],STDIN_FILENO); //STDIN now refers to pipefd[0]
        if(dp<0){ //there is an error with dup2
            perror("dup error");
            exit(1);
        }
        close(pipefd[0]); //close "read" before execvp
        ex=execvp(arglist[i+1], &arglist[i+1]); //we want to pass execvp the command after '|' (i_th index)
        if(ex==-1){ //there is an error with execvp
            perror("execvp error");
            exit(1);
        }

    }

    //parent - shell
    //close pipe in parent process
    close(pipefd[0]);
    close((pipefd[1]));

    //avoid zombies - wait twice, for each child process
    if(waitpid(-1,NULL,0)==-1){ //if there is an error with waitpid
        if(error_handler()==-1){ //if there is a "real" error (errno!=ECHILD, EINTR)
            perror("waitpid error");
            return 0;
        }
    }
    if(waitpid(-1,NULL,0)==-1){ //if there is an error with waitpid
        if(error_handler()==-1){ //if there is a "real" error (errno!=ECHILD, EINTR)
            perror("waitpid error");
            return 0;
        }
    }
    return 1; //success

}











int process_arglist(int count, char **arglist) {

    //look for a special symbol in arglist - '|' or '>' or '&'
    for(int i=0; i<count; i++){
        if (strcmp(arglist[i],"|")==0){ //perform a single piping
            return single_piping(arglist,i);
        }
        if (strcmp(arglist[i],">")==0){ //execute a command with output redirecting
            arglist[i]=NULL; //we will not pass the '>' sign and the output file to execvp...
            return executing_commands(arglist  ,0,1, i+1);
        }
    }
    if (strcmp(arglist[count-1],"&")==0){ //execute a command in the background
        arglist[count-1]=NULL; //we will not pass the '&' sign to execvp...
        return executing_commands(arglist, 1,0,0); //fileIndex is meaningless
    }
    else{ //execute a 'regular' command
        return executing_commands(arglist,0, 0,0); //fileIndex is meaningless
    }
}









int finalize(void){
    return 0;

}