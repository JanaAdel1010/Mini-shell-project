
/*
 * CS-413 Spring 98
 * shell.y: parser for shell
 *
 * This parser compiles the following grammar:
 *
 *	cmd [arg]* [> filename]
 *
 * you must extend it to understand the complete shell grammar
 *
 */
%union	{
		char   *string_val;
	}
%token	<string_val> WORD

%token 	NOTOKEN GREAT APPEND LOW PIPE BG NEWLINE BGAPPEND GREATAMP



%{
extern "C" 
{
	int yylex();
	void yyerror (char const *s);
}
#define yylex yylex
#include <stdio.h>
#include "command.h"
%}

%%

goal:	
	commands
	;

commands: 
	command
	| commands command 
	;

command: simple_command
	|
	;

simple_command:	
	command_and_args piping iomodifier_opt background NEWLINE {
		printf("   Yacc: Execute command\n");
		Command::_currentCommand.execute();
	}
	| NEWLINE 
	| error NEWLINE { yyerrok; }
	;
piping:
	PIPE piped_command{
		printf("yacc:insert piping\n");
	}
	|
	;
piped_command:
	command_and_args piping
	;
background:
	BG {
		printf("yacc running in background\n");
		Command::_currentCommand._background=1;
	}
	|
		{Command::_currentCommand._background=0;}
	;
command_and_args:
	command_word arg_list {
		Command::_currentCommand.
			insertSimpleCommand( Command::_currentSimpleCommand );
	}
	;

arg_list:
	arg_list argument
	| /* can be empty */
	;

argument:
	WORD {
               printf("   Yacc: insert argument \"%s\"\n", $1);

	       Command::_currentSimpleCommand->insertArgument( $1 );\
	}
	;

command_word:
	WORD {
               printf("   Yacc: insert command \"%s\"\n", $1);
	       
	       Command::_currentSimpleCommand = new SimpleCommand();
	       Command::_currentSimpleCommand->insertArgument( $1 );
	}
	;

iomodifier_opt:
	iomodifier_opt GREAT WORD {
		printf("   Yacc: insert output \"%s\"\n", $3);
		Command::_currentCommand._outFile = $3;
	}
	|
	 iomodifier_opt APPEND WORD {
		printf("   Yacc: insert append \"%s\"\n", $3);
		Command::_currentCommand._outFile = $3;
		Command::_currentCommand._append=1;
	}
	| iomodifier_opt LOW WORD {
		printf("   Yacc: insert input \"%s\"\n", $3);
		Command::_currentCommand._outFile = $3;
	}
	| iomodifier_opt BGAPPEND WORD {
		printf("   Yacc: insert error append \"%s\"\n", $3);
		Command::_currentCommand._errFile=$3;
		Command::_currentCommand._append=1;
	}
	| iomodifier_opt GREATAMP WORD {
		printf("   Yacc: insert output and error  \"%s\"\n", $3);
		Command::_currentCommand._errFile=$3;
	}
	|
		
	;

%%

void
yyerror(const char * s)
{
	fprintf(stderr,"%s", s);
}

#if 0
main()
{
	yyparse();
}
#endif
