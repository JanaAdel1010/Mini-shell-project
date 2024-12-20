
/*
 * CS354: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include "command.h"

void Log(int pid)
{
	int log_fd = open("termination_log.txt", O_WRONLY | O_APPEND | O_CREAT, 0666);
	if (log_fd == -1)
	{
		perror("open log file");
		return;
	}

	time_t now = time(NULL);
	char *timestamp = ctime(&now);
	timestamp[strlen(timestamp) - 1] = '\0';

	char log_entry[256];
	snprintf(log_entry, sizeof(log_entry), "Child with PID %d terminated at %s\n", pid, timestamp);

	write(log_fd, log_entry, strlen(log_entry));

	close(log_fd);
}

void sigchld_handler(int signum)
{
	int status;
	pid_t pid;
	while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
	{
		Log(pid);
	}
}
SimpleCommand::SimpleCommand()
{
	// Creat available space for 5 arguments
	_numberOfAvailableArguments = 5;
	_numberOfArguments = 0;
	_arguments = (char **) malloc( _numberOfAvailableArguments * sizeof( char * ) );
}

void
SimpleCommand::insertArgument( char * argument )
{
	if ( _numberOfAvailableArguments == _numberOfArguments  + 1 ) {
		// Double the available space
		_numberOfAvailableArguments *= 2;
		_arguments = (char **) realloc( _arguments,
				  _numberOfAvailableArguments * sizeof( char * ) );
	}
	
	_arguments[ _numberOfArguments ] = argument;

	// Add NULL argument at the end
	_arguments[ _numberOfArguments + 1] = NULL;
	
	_numberOfArguments++;
}

Command::Command()
{
	// Create available space for one simple command
	_numberOfAvailableSimpleCommands = 1;
	_simpleCommands = (SimpleCommand **)
		malloc( _numberOfSimpleCommands * sizeof( SimpleCommand * ) );

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
	_append=0;
}

void
Command::insertSimpleCommand( SimpleCommand * simpleCommand )
{
	if ( _numberOfAvailableSimpleCommands == _numberOfSimpleCommands ) {
		_numberOfAvailableSimpleCommands *= 2;
		_simpleCommands = (SimpleCommand **) realloc( _simpleCommands,
			 _numberOfAvailableSimpleCommands * sizeof( SimpleCommand * ) );
	}
	
	_simpleCommands[ _numberOfSimpleCommands ] = simpleCommand;
	_numberOfSimpleCommands++;
}

void
Command:: clear()
{
	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) {
		for ( int j = 0; j < _simpleCommands[ i ]->_numberOfArguments; j ++ ) {
			free ( _simpleCommands[ i ]->_arguments[ j ] );
		}
		
		free ( _simpleCommands[ i ]->_arguments );
		free ( _simpleCommands[ i ] );
	}

	if ( _outFile ) {
		free( _outFile );
	}

	if ( _inputFile ) {
		free( _inputFile );
	}

	if ( _errFile ) {
		free( _errFile );
	}

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
	_append=0;
}

void
Command::print()
{
	printf("\n\n");
	printf("              COMMAND TABLE                \n");
	printf("\n");
	printf("  #   Simple Commands\n");
	printf("  --- ----------------------------------------------------------\n");
	
	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) {
		printf("  %-3d ", i );
		for ( int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++ ) {
			printf("\"%s\" \t", _simpleCommands[i]->_arguments[ j ] );
		}
	}

	printf( "\n\n" );
	printf( "  Output       Input        Error        Background\n" );
	printf( "  ------------ ------------ ------------ ------------\n" );
	printf( "  %-12s %-12s %-12s %-12s\n", _outFile?_outFile:"default",
		_inputFile?_inputFile:"default", _errFile?_errFile:"default",
		_background?"YES":"NO");
	printf( "\n\n" );
	
}


void
Command::execute()
{
	// Don't do anything if there are no simple commands
	if ( _numberOfSimpleCommands == 0 ) {
		prompt();
		return;
	}
	print();


	if(_numberOfSimpleCommands==1 and strcmp(_simpleCommands[0]->_arguments[0],"exit")==0)
	{
		printf("Good bye !!\n");
		clear();
		exit(0);
	}
	if(strcmp(_simpleCommands[0]->_arguments[0],"cd")==0)
	{
		const char *destdir;
		if(_simpleCommands[0]->_numberOfArguments >1)
		{
			destdir=_simpleCommands[0]->_arguments[1];
		}
		else
		{
			destdir=getenv("HOME");

			if(!destdir)
			{
				fprintf(stderr, "cd: HOME environment variable is not set\n");
				clear();
				prompt();
				return;
			}
		}
		if(chdir(destdir)!=0)
		{
			perror("cd");
		}
		clear();
		prompt();
		return;
	}	

	// Add execution here
	// For every simple command fork a new process
	// Setup i/o redirection
	// and call exec

	int pipes[2 * (_numberOfSimpleCommands - 1)];

	for(int i = 0 ;i < _numberOfSimpleCommands - 1; i++)
	{
		if(pipe(pipes + i *2) == -1)
		{
			perror("pipe");
			exit(1);
		}
	}
	pid_t last_pid=-1;
	for (int i=0;i< _numberOfSimpleCommands;i++)
	{
		int pid=fork();
		if(pid == -1)
		{
			perror("fork");
			exit(2);
		}
		if (pid == 0)
		{

			if(_inputFile)
			{
				int fd_in=open(_inputFile, O_RDONLY);
				if (fd_in < 0)
				{
					perror("my shell:can't open file");
					exit(1);
				}
				dup2(fd_in,0);
				close(fd_in);
			}
			if (i > 0)
			{
				dup2(pipes[(i-1) * 2],0);
			}
			if (i == _numberOfSimpleCommands-1)
			{
				if(_outFile)
			{
				int flags= O_CREAT | O_WRONLY | (_append ? O_APPEND : O_TRUNC);
				int fd_out= open(_outFile,flags, 0644);
				if(fd_out<0)
				{
					perror("myshell:can't open output file");
					exit(1);
				}
				dup2(fd_out,1);
				close(fd_out);
			}
			}else{
				dup2(pipes[i * 2 + 1], 1);
			}
			
			if(_errFile)
			{
				int flags = O_CREAT | O_WRONLY | (_append ? O_APPEND : O_TRUNC);
				int fd_err = open(_errFile, flags, 0644);
				if (fd_err < 0) {
					perror("myshell: cannot open error file");
					exit(1);
				}
				dup2(fd_err, 2);
				close(fd_err);
			}
			for(int j =0;j<2*(_numberOfSimpleCommands-1);j++)
			{
				close(pipes[j]);
			}

			execvp(_simpleCommands[i]->_arguments[0],_simpleCommands[i]->_arguments);
			perror("execvp");
			_exit(1);
		}
		else 
		{
			last_pid=pid;
		}}
		for(int j =0;j<2*(_numberOfSimpleCommands-1);j++)
			{
				close(pipes[j]);
			}

	if(! _background)
	{
		int status;
		waitpid(last_pid,NULL,0);
		Log(last_pid);
	}
	// Clear to prepare for next command
	clear();
	
	// Print new prompt
	prompt();
}

// Shell implementation

void
Command::prompt()
{
	printf("myshell>");
	fflush(stdout);
}

Command Command::_currentCommand;
SimpleCommand * Command::_currentSimpleCommand;

int yyparse(void);
void handle_ctr_c(int sig)
{
	printf("\n shell will ignore ctr\n");
	printf("To terminate enter exit\n");
	Command::_currentCommand.prompt();
}
main()
{
	if(signal(SIGCHLD,sigchld_handler)== SIG_ERR)
	{
		perror("SIGCHILD");
		exit(1);
	}
	if(signal(SIGINT,handle_ctr_c)==SIG_ERR)
	{
		perror("SIGNIT");
		exit(1);
	}

	Command::_currentCommand.prompt();
	yyparse();
	return 0;
}

