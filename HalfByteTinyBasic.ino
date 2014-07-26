// TinyBASIC.cpp : An implementation of TinyBASIC in C
//
// Original Author : Mike Field - hamster@snap.net.nz
// This Version: George Gray - halfbyteblog.wordpress.com
// Based on TinyBasic for 68000, by Gordon Brandly
// (see http://members.shaw.ca/gbrandly/68ktinyb.html)
//
// which itself was Derived from Palo Alto Tiny BASIC as 
// published in the May 1976 issue of Dr. Dobb's Journal.  
// 
// 0.03 21/01/2011 : Added INPUT routine M
//                 : Reorganised memory layout
//                 : Expanded all error messages
//                 : Break key added
//                 : Removed the calls to printf (left by debugging)
//2013: began working on modifying for console-George Gray, HalfByte
//2014: heavily modified for HalfByte Console.
//added functions to read Nunchuck, uart
//added graphics statements
//added sound
//fixed POKE
//added SERIAL and PIN access



//#include <SD.h>
#include <i2cmaster.h>
#include <nunchuck.h>
#include <font4x6g.h>
#include <TVout.h>
#include <PS2Keyboard.h>
#include <EEPROM.h>

TVout TV;

boolean nunchuk;
Nunchuck nunchuck;
const int DataPin = 2;
const int IRQpin =  3;
int outswitch = false;
int echo = true;
int stopFlag = false;

//File file;

PS2Keyboard keyboard;

#ifndef ARDUINO
#include "stdafx.h"
#include <conio.h>
#endif 

// ASCII Characters
#define CR	'\r'
#define NL	'\n'
#define TAB	'\t'
#define BELL	'\b'
#define DEL	'\127'
#define SPACE   ' '
#define CTRLC	0x19
#define CTRLH	0x7F
#define CTRLS	0x13
#define CTRLX	0x18

typedef short unsigned LINENUM;


/***********************************************************/
// Keyword table and constants - the last character has 0x80 added to it
static unsigned char keywords[] PROGMEM = {
	'L','I','S','T'+0x80,
	'L','O','A','D'+0x80,
	'N','E','W'+0x80,
	'R','U','N'+0x80,
	'S','A','V','E'+0x80,
	'N','E','X','T'+0x80,
	'L','E','T'+0x80,
	'I','F'+0x80,
	'G','O','T','O'+0x80,
	'G','O','S','U','B'+0x80,
	'R','E','T','U','R','N'+0x80,
	'R','E','M'+0x80,
	'F','O','R'+0x80,
	'I','N','P','U','T'+0x80,
	'P','R','I','N','T'+0x80,
	'P','O','K','E'+0x80,
	'S','T','O','P'+0x80,
	'B','Y','E'+0x80,
        'C','L','S'+0x80,
        'C','U','R','S','O','R'+0x80,
        'S','E','T'+0x80,
        'R','E','S','E','T'+0x80,
        '@'+0x80,
        '?'+0x80,
        'F','I','L','E','S'+0x80,
        'A','W','R','I','T','E'+0x80,
        'D','W','R','I','T','E'+0x80,
        'M','E','M'+0x80,
        'B','O','X'+0x80,
        'L','I','N','E'+0x80,
        'C','I','R','C','L','E'+0x80,
        'T','O','N','E'+0x80,  
        'D','E','L','A','Y'+0x80,  
        'S','H','I','F','T'+0x80, 
        'I','N','V','E','R','T'+0x80, 
        '#'+0x80,
        'O','U','T'+0x80,
        'S','P','R','I','N','T'+0x80,
        'E','C','H','O'+0x80,
        'C','L','E','A','R'+0x80,
        'C','E','N','T','E','R'+0x80,
	0
};

#define KW_LIST		0
#define KW_LOAD		1
#define KW_NEW		2
#define KW_RUN		3
#define KW_SAVE		4
#define KW_NEXT		5
#define KW_LET		6
#define KW_IF		7
#define KW_GOTO		8
#define KW_GOSUB	9
#define KW_RETURN	10
#define KW_REM		11
#define KW_FOR		12
#define KW_INPUT	13
#define KW_PRINT	14
#define KW_POKE		15
#define KW_STOP		16
#define KW_BYE		17
#define KW_CLS          18
#define KW_CURSOR       19
#define KW_SET          20
#define KW_RESET        21
#define KW_AT           22
#define KW_QMARK        23
#define KW_FILES        24
#define KW_AWRITE       25
#define KW_DWRITE       26 
#define KW_MEM          27
#define KW_BOX          28
#define KW_LINE         29
#define KW_CIRCLE       30
#define KW_TONE         31
#define KW_DELAY        32
#define KW_SHIFT        33
#define KW_INVERT       34
#define KW_HASHTAG      35
#define KW_OUT          36
#define KW_SPRINT       37
#define KW_ECHO         38
#define KW_CLEAR        39
#define KW_CENTER       40
#define KW_DEFAULT	41

struct stack_for_frame {
	char frame_type;
	char for_var;
	short int terminal;
	short int step;
	unsigned char *current_line;
	unsigned char *txtpos;
};

struct stack_gosub_frame {
	char frame_type;
	unsigned char *current_line;
	unsigned char *txtpos;
};

static unsigned char func_tab[] PROGMEM = {
	'P','E','E','K'+0x80,
	'A','B','S'+0x80,
	'R','N','D'+0x80,
	'P','A','D'+0x80,
	'G','E','T','S','P','I'+0x80,
	'I','N','K','E','Y'+0x80,
	'C','H','R'+0x80,
	'@'+0x80,
	'I','N'+0x80,
	'M','E','M'+0x80,
        'T','O','P'+0x80,
        'A','R','E','A','D'+0x80,
        'D','R','E','A','D'+0x80,
        'G','E','T'+0x80,
	0
};
#define FUNC_PEEK  	0
#define FUNC_ABS  	1
#define FUNC_RND 	2
#define FUNC_PAD	3
#define FUNC_READ	4
#define FUNC_INKEY	5
#define FUNC_CHR	6
#define FUNC_AT		7
#define FUNC_UARTIN	8
#define FUNC_MEM	9
#define FUNC_TOP        10
#define FUNC_AREAD      11
#define FUNC_DREAD      12
#define FUNC_GET        13
#define FUNC_UNKNOWN    14

static unsigned char to_tab[] PROGMEM = {
	'T','O'+0x80,
	0
};

static unsigned char step_tab[] PROGMEM = {
	'S','T','E','P'+0x80,
	0
};

static unsigned char relop_tab[] PROGMEM = {
	'>','='+0x80,
	'<','>'+0x80,
	'>'+0x80,
	'='+0x80,
	'<','='+0x80,
	'<'+0x80,
	0
};

#define RELOP_GE		0
#define RELOP_NE		1
#define RELOP_GT		2
#define RELOP_EQ		3
#define RELOP_LE		4
#define RELOP_LT		5
#define RELOP_UNKNOWN	        6

static unsigned char highlow_tab[] PROGMEM = { 
  'H','I','G','H'+0x80,
  'H','I'+0x80,
  'L','O','W'+0x80,
  'L','O'+0x80,
  0
};

#define HIGHLOW_HIGH    1
#define HIGHLOW_UNKNOWN 4

#define VAR_SIZE sizeof(short int) // Size of variables in bytes

static unsigned char memory[1054];
static unsigned char *txtpos,*list_line;
static unsigned char expression_error;
static unsigned char *tempsp;
static unsigned char *stack_limit;
static unsigned char *program_start;
static unsigned char *program_end;
static unsigned char *stack; // Software stack for things that should go on the CPU stack
static unsigned char *variables_table;
static unsigned char *current_line;
static unsigned char *spt;
#define STACK_GOSUB_FLAG 'G'
#define STACK_FOR_FLAG 'F'
static unsigned char table_index;
static LINENUM linenum;

static const unsigned char okmsg[]		PROGMEM= "Ready";
static const unsigned char badlinemsg[]		PROGMEM= "Bad line #";
static const unsigned char invalidexprmsg[]     PROGMEM= "Expr error";
static const unsigned char syntaxmsg[]          PROGMEM= "Syntax error";
static const unsigned char badinputmsg[]        PROGMEM= "\nBad Number";
static const unsigned char nomemmsg[]	        PROGMEM= "No memory!";
static const unsigned char initmsg[]	        PROGMEM= "Half-Byte\nTiny Basic";
static const unsigned char memorymsg[]	        PROGMEM= " bytes free.";
static const unsigned char breakmsg[]	        PROGMEM= "Break!";
static const unsigned char stackstuffedmsg[]    PROGMEM= "Stack!\n";
static const unsigned char unimplimentedmsg[]	PROGMEM= "Not yet";
static const unsigned char backspacemsg[]	PROGMEM= "\b \b";

static int inchar(void);
static void outchar(unsigned char c);
static void line_terminator(void);
static short int expression(void);
static unsigned char breakcheck(void);
/***************************************************************************/
static void ignore_blanks(void)
{
	while(*txtpos == SPACE || *txtpos == TAB)
		txtpos++;
}

/***************************************************************************/
static void scantable(unsigned char *table)
{
	int i = 0;
	ignore_blanks();
	table_index = 0;
	while(1)
	{
	    // Run out of table entries?
	    if(pgm_read_byte(table) == 0)
            return;

		// Do we match this character?
		if(txtpos[i] == pgm_read_byte( table ))
		{
			i++;
			table++;
		}
		else
		{
			// do we match the last character of keywork (with 0x80 added)? If so, return
			if(txtpos[i]+0x80 == pgm_read_byte( table ))
			{
				txtpos += i+1;  // Advance the pointer to following the keyword
				ignore_blanks();
				return;
			}

			// Forward to the end of this keyword
			while((pgm_read_byte( table ) & 0x80) == 0)
				table++;

			// Now move on to the first character of the next word, and reset the position index
			table++;
			table_index++;
			i = 0;
		}
	}
}

/***************************************************************************/
static void pushb(unsigned char b)
{
	spt--;
	*spt = b;
}

/***************************************************************************/
static unsigned char popb()
{
	unsigned char b;
	b = *spt;
	spt++;
	return b;
}

/***************************************************************************/
static void printnum(int num)
{
	int digits = 0;

	if(num < 0)
	{
		num = -num;
		outchar('-');
	}

	do {
		pushb(num%10+'0');
		num = num/10;
		digits++;
	}
	while (num > 0);

	while(digits > 0)
	{
		outchar(popb());
		digits--;
	}
}
/***************************************************************************/
static unsigned short testnum(void)
{
	unsigned short num = 0;
	ignore_blanks();
	
	while(*txtpos>= '0' && *txtpos <= '9' )
	{
		// Trap overflows
		if(num >= 0xFFFF/10)
		{
			num = 0xFFFF;
			break;
		}

		num = num *10 + *txtpos - '0';
		txtpos++;
	}
	return	num;
}

/***************************************************************************/
unsigned char check_statement_end(void)
{
	ignore_blanks();
	return (*txtpos == NL) || (*txtpos == ':');
}

/***************************************************************************/
/*static void printmsgNoNL(const unsigned char *msg)
{
	while(*msg)
	{
		outchar(*msg);
		msg++;
	}
}
*/
/***************************************************************************/
void printmsgNoNL(const unsigned char *msg)
{
  while( pgm_read_byte( msg ) != 0 ) {
    outchar( pgm_read_byte( msg++ ) );
  };
}
/***************************************************************************/
static unsigned char print_quoted_string(void)
{
	int i=0;
	unsigned char delim = *txtpos;
	if(delim != '"' && delim != '\'')
		return 0;
	txtpos++;

	// Check we have a closing delimiter
	while(txtpos[i] != delim)
	{
		if(txtpos[i] == NL)
			return 0;
		i++;
	}

	// Print the characters
	while(*txtpos != delim)
	{
		outchar(*txtpos);
		txtpos++;
	}
	txtpos++; // Skip over the last delimiter
	ignore_blanks();

	return 1;
}

/***************************************************************************/
static void printmsg(const unsigned char *msg)
{
    printmsgNoNL(msg);
    line_terminator();
}

/***************************************************************************/
unsigned char getln(char prompt)
{
	outchar(prompt);
	txtpos = program_end+sizeof(LINENUM);

	while(1)
	{
		char c = inchar();
		switch(c)
		{
			case CR:
			case NL:
                                line_terminator();
				// Terminate all strings with a NL
				txtpos[0] = NL;
				return 1;
			case CTRLC:
				return 0;
			case CTRLH:
				if(txtpos == program_end)
					break;
				txtpos--;
				printmsgNoNL(backspacemsg);
				break;
			default:
				// We need to leave at least one space to allow us to shuffle the line into order
				if(txtpos == spt-2)
					outchar(BELL);
				else
				{
					txtpos[0] = c;
					txtpos++;
					outchar(c);
				}
		}
	}
}

/***************************************************************************/
static unsigned char *findline(void)
{
	unsigned char *line = program_start;
	while(1)
	{
		if(line == program_end)
			return line;

		if(((LINENUM *)line)[0] >= linenum)
			return line;

		// Add the line length onto the current address, to get to the next line;
		line += line[sizeof(LINENUM)];
	}
}

/***************************************************************************/
static void toUppercaseBuffer(void)
{
	unsigned char *c = program_end+sizeof(LINENUM);
	unsigned char quote = 0;

	while(*c != NL)
	{
		// Are we in a quoted string?
		if(*c == quote)
			quote = 0;
		else if(*c == '"' || *c == '\'')
			quote = *c;
		else if(quote == 0 && *c >= 'a' && *c <= 'z')
			*c = *c + 'A' - 'a';
		c++;
	}
}

/***************************************************************************/
void printline()
{
	LINENUM line_num;
	
	line_num = *((LINENUM *)(list_line));
        list_line += sizeof(LINENUM) + sizeof(char);

	// Output the line */
	printnum(line_num);
	outchar(' ');
	while(*list_line != NL)
    {
		outchar(*list_line);
		list_line++;
	}
	list_line++;
	line_terminator();
}

/***************************************************************************/
static short int expr4(void)
{
	short int a = 0;
        short int b = 0;
        
        // fix provided by Jurg Wullschleger wullschleger@gmail.com
        // fixes whitespace and unary operations
        ignore_blanks();
      
        if( *txtpos == '-' ) {
          txtpos++;
          return -expr4();
        }
        // end fix

	if(*txtpos == '0')
	{
		txtpos++;
		a = 0;
		goto success;
	}

	if(*txtpos >= '1' && *txtpos <= '9')
	{
		do 	{
			a = a*10 + *txtpos - '0';
			txtpos++;
		} while(*txtpos >= '0' && *txtpos <= '9');
			goto success;
	}

	// Is it a function or variable reference?
	if(txtpos[0] >= 'A' && txtpos[0] <= 'Z')
	{
		// Is it a variable reference (single alpha)
		if(txtpos[1] < 'A' || txtpos[1] > 'Z')
		{
			a = ((short int *)variables_table)[*txtpos - 'A'];
			txtpos++;
			return a;
		}

		// Is it a function with a single parameter
		scantable(func_tab);
		if(table_index == FUNC_UNKNOWN)
			goto expr4_error;

		unsigned char f = table_index;

		if(*txtpos != '(')
			goto expr4_error;

		txtpos++;
		a = expression();
                // check for a comma
                ignore_blanks();
                if (*txtpos != ',')
                {
                  //txtpos++;
                  ignore_blanks;
		  if(*txtpos != ')')
		     goto expr4_error;
                }
                else {
                      txtpos++;
                      b=expression();
                      ignore_blanks;
    		      if(*txtpos != ')')
    		        goto expr4_error;
                }
                
		txtpos++;
		switch(f)
		{
                 	case FUNC_AT:
                                return memory[a];
			case FUNC_PEEK:
				return memory[a];
				
			case FUNC_ABS:
				if(a < 0)
				  return  -a;
				return a;

			case FUNC_RND:
                                int tempR;
				// generate random number between 0 and x			
				tempR=random(a);
                                if (tempR<0)
                                    tempR=-tempR;

				return tempR;
				
                        case FUNC_TOP:
                                return (short int)program_end;
                                
			case FUNC_PAD:
				// read gamepad 0=x axis, 1=y axis,2=c button,3=z button,4=accelerometer x,5=accelerometer y,6=accelerometer z
                                int GamepadVal;
                                
				switch(a)
				{
					case 0:
					nunchuck.update(); 					
					{						
						GamepadVal = nunchuck.joy_x(); //Nunchuk.getJoystickX();
						a = (unsigned short int)GamepadVal; 
						if (GamepadVal!=0) break;
					}
					case 1:
					nunchuck.update();					
					{
						GamepadVal = nunchuck.joy_y(); //Nunchuk.getJoystickY();
						a = (unsigned short int)GamepadVal; 
						if (GamepadVal!=0) break;
					}
                                        case 2:
                                        nunchuck.update();
                                        {
                                                GamepadVal = nunchuck.button_c(); //Nunchuk.getButtonC();
                                                a = (unsigned short int)GamepadVal;
                                                break;
                                        }
                                        case 3:
                                        nunchuck.update();
                                        {
                                                GamepadVal = nunchuck.button_z(); //Nunchuk.getButtonC();
                                                a = (unsigned short int)GamepadVal;
                                                break;
                                        }
                                        case 4:
                                        nunchuck.update();
                                        {
                                                GamepadVal = nunchuck.acc_x(); //Nunchuk.getButtonC();
                                                a = (unsigned short int)GamepadVal;
                                                if (GamepadVal!=0) break;
                                        } 
                                        case 5:
                                        nunchuck.update();
                                        {
                                                GamepadVal = nunchuck.acc_y(); //Nunchuk.getButtonC();
                                                a = (unsigned short int)GamepadVal;
                                                if (GamepadVal!=0) break;
                                        }                                        
                                        case 6:
                                        nunchuck.update();
                                        {
                                                GamepadVal = nunchuck.acc_z(); //Nunchuk.getButtonC();
                                                a = (unsigned short int)GamepadVal;
                                                if (GamepadVal!=0) break;
                                        }
				}
				goto success;

			case FUNC_READ:
				// read a byte from the SPI
				//a = SPI_Read(0);
				goto success;
			case FUNC_INKEY:
				// grab a key from keyboard, if there				
                                return (char)keyboard.read();
				
			case FUNC_CHR:
				//output ascii
				{
					outchar(a);
					expression_error=15;
					goto success;
				}
			case FUNC_UARTIN:
				// get a byte of serial
			  {
			    return Serial.read();		    
			  }
                        case FUNC_AREAD:
                          pinMode( a, INPUT );
                          return analogRead( a );                        
                        case FUNC_DREAD:
                          pinMode( a, INPUT );
                          return digitalRead( a );
			case FUNC_MEM:
				// Return memory remaining			 
			  return spt-program_end;
                        case FUNC_GET:
                            int x,y;
                            x=a;
                            y=b;
                            return (TV.get_pixel(x,y));
		}

	}

	if(*txtpos == '(')
	{
		txtpos++;
		a = expression();
		if(*txtpos != ')')
			goto expr4_error;

		txtpos++;
		goto success;
	}

expr4_error:
	expression_error = 1;
success:
	ignore_blanks();
	return a;
}

/***************************************************************************/
static short int expr3(void)
{
	short int a,b;

	a = expr4();
	while(1)
	{
		if(*txtpos == '*')
		{
			txtpos++;
			b = expr4();
			a *= b;
		}
		else if(*txtpos == '/')
		{
			txtpos++;
			b = expr4();
			if(b != 0)
				a /= b;
			else
				expression_error = 1;
		}
                else if(*txtpos == '^')
                {
                      txtpos++;
                       b = expr4();
                       if(b != 0)
                         return pow(a,b);
                }
		else
			return a;
	}
}

/***************************************************************************/
static short int expr2(void)
{
	short int a,b;

	if(*txtpos == '-' || *txtpos == '+')
		a = 0;
	else
		a = expr3();

	while(1)
	{
		if(*txtpos == '-')
		{
			txtpos++;
			b = expr3();
			a -= b;
		}
		else if(*txtpos == '+')
		{
			txtpos++;
			b = expr3();
			a += b;
		}
		else
			return a;
	}
}
/***************************************************************************/
static short int expression(void)
{
	short int a,b;

	a = expr2();
	// Check if we have an error
	if(expression_error)	return a;

	scantable(relop_tab);
	if(table_index == RELOP_UNKNOWN)
		return a;
	
	switch(table_index)
	{
	case RELOP_GE:
		b = expr2();
		if(a >= b) return 1;
		break;
	case RELOP_NE:
		b = expr2();
		if(a != b) return 1;
		break;
	case RELOP_GT:
		b = expr2();
		if(a > b) return 1;
		break;
	case RELOP_EQ:
		b = expr2();
		if(a == b) return 1;
		break;
	case RELOP_LE:
		b = expr2();
		if(a <= b) return 1;
		break;
	case RELOP_LT:
		b = expr2();
		if(a < b) return 1;
		break;
	}
	return 0;
}

/***************************************************************************/
void loop()
{
	unsigned char *start;
	unsigned char *newEnd;
	unsigned char linelen;
        int clr_count;
	boolean isDigital;

	variables_table = memory;
	program_start = memory + 27*VAR_SIZE;
	program_end = program_start;
	spt = memory+sizeof(memory);  
	printmsg(initmsg);
	printnum(spt-program_end);
	printmsg(memorymsg);

warmstart:
	// this signifies that it is running in 'direct' mode.
	current_line = 0;
	spt = memory+sizeof(memory);  
	printmsg(okmsg);

prompt:
	while(!getln('>'))
		line_terminator();
	toUppercaseBuffer();

	txtpos = program_end+sizeof(unsigned short);

	// Find the end of the freshly entered line
	while(*txtpos != NL)
		txtpos++;

	// Move it to the end of program_memory
	{
		unsigned char *dest;
		dest = spt-1;
		while(1)
		{
			*dest = *txtpos;
			if(txtpos == program_end+sizeof(unsigned short))
				break;
			dest--;
			txtpos--;
		}
		txtpos = dest;
	}

	// Now see if we have a line number
	linenum = testnum();
	ignore_blanks();
	if(linenum == 0)
		goto direct;

	if(linenum == 0xFFFF)
		goto badline;

	// Find the length of what is left, including the (yet-to-be-populated) line header
	linelen = 0;
	while(txtpos[linelen] != NL)
		linelen++;
	linelen++; // Include the NL in the line length
	linelen += sizeof(unsigned short)+sizeof(char); // Add space for the line number and line length

	// Now we have the number, add the line header.
	txtpos -= 3;
	*((unsigned short *)txtpos) = linenum;
	txtpos[sizeof(LINENUM)] = linelen;


	// Merge it into the rest of the program
	start = findline();

	// If a line with that number exists, then remove it
	if(start != program_end && *((LINENUM *)start) == linenum)
	{
		unsigned char *dest, *from;
		unsigned tomove;

		from = start + start[sizeof(LINENUM)];
		dest = start;

		tomove = program_end - from;
		while( tomove > 0)
		{
			*dest = *from;
			from++;
			dest++;
			tomove--;
		}	
		program_end = dest;
	}

	if(txtpos[sizeof(LINENUM)+sizeof(char)] == NL) // If the line has no txt, it was just a delete
		goto prompt;



	// Make room for the new line, either all in one hit or lots of little shuffles
	while(linelen > 0)
	{	
		unsigned int tomove;
		unsigned char *from,*dest;
		unsigned int space_to_make;
	
		space_to_make = txtpos - program_end;

		if(space_to_make > linelen)
			space_to_make = linelen;
		newEnd = program_end+space_to_make;
		tomove = program_end - start;


		// Source and destination - as these areas may overlap we need to move bottom up
		from = program_end;
		dest = newEnd;
		while(tomove > 0)
		{
			from--;
			dest--;
			*dest = *from;
			tomove--;
		}

		// Copy over the bytes into the new space
		for(tomove = 0; tomove < space_to_make; tomove++)
		{
			*start = *txtpos;
			txtpos++;
			start++;
			linelen--;
		}
		program_end = newEnd;
	}
	goto prompt;

unimplemented:
	printmsg(unimplimentedmsg);
	goto prompt;

badline:	
	printmsg(badlinemsg);
	goto prompt;
invalidexpr:
	printmsg(invalidexprmsg);
	goto prompt;
syntaxerror:
	printmsg(syntaxmsg);
	if(current_line != (void *)0)
	{
           unsigned char tmp = *txtpos;
		   if(*txtpos != NL)
				*txtpos = '^';
           list_line = current_line;
           printline();
           *txtpos = tmp;
	}
    line_terminator();
	goto prompt;

stackstuffed:	
	printmsg(stackstuffedmsg);
	goto warmstart;
nomem:	
	printmsg(nomemmsg);
	goto warmstart;

run_next_statement:
        if(breakcheck())
        {
          printmsg(breakmsg);
          goto warmstart;
        }
	while(*txtpos == ':')
		txtpos++;
	ignore_blanks();
	if(*txtpos == NL)
		goto execnextline;
	goto interperateAtTxtpos;

direct: 
	txtpos = program_end+sizeof(LINENUM);
	if(*txtpos == NL)
		goto prompt;

interperateAtTxtpos:
        if(breakcheck())
        {
          printmsg(breakmsg);
          goto warmstart;
        }

	scantable(keywords);
	ignore_blanks();

	switch(table_index)
	{
		case KW_LIST:
			goto list;
		case KW_LOAD:
                        goto eLoad;			
		case KW_NEW:
			if(txtpos[0] != NL)
			  goto syntaxerror;
			program_end = program_start;
			goto prompt;
		case KW_RUN:
			current_line = program_start;
			goto execline;
		case KW_SAVE:
                        goto eSave;			
		case KW_NEXT:
			goto next;
		case KW_LET:
			goto assignment;
		case KW_IF:
			{
			short int val;
			expression_error = 0;
			val = expression();
			if(expression_error || *txtpos == NL)
				goto invalidexpr;
			if(val != 0)
				goto interperateAtTxtpos;
			goto execnextline;
			}
		case KW_GOTO:
			expression_error = 0;
			linenum = expression();
			if(expression_error || *txtpos != NL)
				goto invalidexpr;
			current_line = findline();
			goto execline;

		case KW_GOSUB:
			goto gosub;
		case KW_RETURN:
			goto gosub_return; 
                case KW_HASHTAG:
		case KW_REM:	
			goto execnextline;	// Ignore line completely
		case KW_FOR:
			goto forloop; 
		case KW_INPUT:
			goto input; 
                case KW_QMARK:
		case KW_PRINT:
			goto print;
                case KW_AT:
		case KW_POKE:
			goto poke;
		case KW_STOP:
			// This is the easy way to end - set the current line to the end of program attempt to run it
			if(txtpos[0] != NL)
				goto syntaxerror;
			current_line = program_end;
			goto execline;
		case KW_BYE:
			// Leave the basic interperater
			return;
                case KW_CLS:
                    TV.clear_screen();
                    goto run_next_statement;
                case KW_CURSOR:
                    goto setcursor;
                case KW_SET:
                    goto setpixel;
                case KW_RESET:
                    goto resetpixel;
                case KW_FILES:
                    //file=SD.open("/");
                    //printDirectory(file);
                    goto execnextline;
                case KW_AWRITE:  // AWRITE <pin>, HIGH|LOW
                    isDigital = false;
                    goto awrite;
                case KW_DWRITE:  // DWRITE <pin>, HIGH|LOW
                    isDigital = true;
                    goto dwrite;
                case KW_MEM:
                    goto mem;  
                case KW_BOX:
                    goto box;
                case KW_LINE:
                    goto line;
                case KW_CIRCLE:
                    goto circle;
                case KW_TONE:
                    goto mTone;
                case KW_DELAY:
                {
                    short int val;
                    expression_error = 0;
                    val = expression();
                    delay( val );
                    goto run_next_statement;
                  }
                case KW_SHIFT:
                    goto shiftscreen;
                case KW_INVERT:
                    TV.invert();
                    goto execnextline;
                case KW_OUT:
                    goto sOut;
                case KW_SPRINT:
                    outswitch=true;
                    goto sprint;
                case KW_ECHO:
                    goto echoonoff;
                case KW_CLEAR:
                    clr_count=0;
                    while (clr_count<27*VAR_SIZE){
                      memory[(int)clr_count]=0;
                      clr_count++;
                    }
                    goto run_next_statement;
                case KW_CENTER:
                    goto centertext;
                case KW_DEFAULT:
		    goto assignment;
		default:
			break;
	}
	
execnextline:
	if(current_line == (void *)0)		// Processing direct commands?
		goto prompt;
	current_line +=	 current_line[sizeof(LINENUM)];

execline:
  	if(current_line == program_end) // Out of lines to run
		goto warmstart;
	txtpos = current_line+sizeof(LINENUM)+sizeof(char);
	goto interperateAtTxtpos;

input:
	{
		unsigned char isneg=0;
		unsigned char *temptxtpos;
		short int *var;
		ignore_blanks();
		if(*txtpos < 'A' || *txtpos > 'Z')
			goto syntaxerror;
		var = ((short int *)variables_table)+*txtpos-'A';
		txtpos++;
		if(!check_statement_end())
			goto syntaxerror;
again:
		temptxtpos = txtpos;
		if(!getln('?'))
			goto warmstart;

		// Go to where the buffer is read
		txtpos = program_end+sizeof(LINENUM);
		if(*txtpos == '-')
		{
			isneg = 1;
			txtpos++;
		}

		*var = 0;
		do 	{
			*var = *var*10 + *txtpos - '0';
			txtpos++;
		} while(*txtpos >= '0' && *txtpos <= '9');
		ignore_blanks();
		if(*txtpos != NL)
		{
			printmsg(badinputmsg);
			goto again;
		}
	
		if(isneg)
			*var = -*var;

		goto run_next_statement;
	}
mem:
                // memory free
                printnum(spt-program_end);
                printmsg(memorymsg);
                goto run_next_statement;
centertext:
        {
                short int x;
                short int y;
                
        }    
sOut:
	{
		short int value;
		short int x;

		// Work out where to put it
		x = checkParm();
                if (stopFlag==true)
                    goto prompt; 

                Serial.write(x);
		// Check that we are at the end of the statement
		if(!check_statement_end())
			goto syntaxerror;
	}
	goto run_next_statement;

                
awrite:         // AWRITE <pin>,val
dwrite:                
                  short int pinNo;
                  short int val;
                  //unsigned char *txtposBak;
              
                  // Get the pin number
                  expression_error = 0;
                  pinNo = checkParm();
              
                  // check for a comma
                  checkForComma();       
                  if (stopFlag==true)
                    goto prompt; 
                    
                  //txtposBak = txtpos; 

                  // and the value (numerical)
                    expression_error = 0;
                    val = expression();
                    //if(expression_error)
                    //  goto syntaxerror;
                  //}
                  pinMode( pinNo, OUTPUT );
                  if( isDigital ) {
                    digitalWrite( pinNo, val );
                  } 
                  else {
                    analogWrite( pinNo, val );
                  }
                
                //expression_error = 15;
                goto run_next_statement;                
                
forloop:
	{
		unsigned char var;
		short int initial, step, terminal;

		if(*txtpos < 'A' || *txtpos > 'Z')
			goto syntaxerror;
		var = *txtpos;
		txtpos++;
		
		scantable(relop_tab);
		if(table_index != RELOP_EQ)
			goto syntaxerror;

		expression_error = 0;
		initial = expression();
		if(expression_error)
			goto invalidexpr;
	
		scantable(to_tab);
		if(table_index != 0)
			goto syntaxerror;
	
		terminal = expression();
		if(expression_error)
			goto invalidexpr;
	
		scantable(step_tab);
		if(table_index == 0)
		{
			step = expression();
			if(expression_error)
				goto invalidexpr;
		}
		else
			step = 1;
		if(!check_statement_end())
			goto syntaxerror;


		if(!expression_error && *txtpos == NL)
		{
			struct stack_for_frame *f;
			if(spt + sizeof(struct stack_for_frame) < stack_limit)
				goto nomem;

			spt -= sizeof(struct stack_for_frame);
			f = (struct stack_for_frame *)spt;
			((short int *)variables_table)[var-'A'] = initial;
			f->frame_type = STACK_FOR_FLAG;
			f->for_var = var;
			f->terminal = terminal;
			f->step     = step;
			f->txtpos   = txtpos;
			f->current_line = current_line;
			goto run_next_statement;
		}
	}
	goto syntaxerror;

gosub:
	expression_error = 0;
	linenum = expression();
	if(expression_error)
		goto invalidexpr;
	if(!expression_error && *txtpos == NL)
	{
		struct stack_gosub_frame *f;
		if(spt + sizeof(struct stack_gosub_frame) < stack_limit)
			goto nomem;

		spt -= sizeof(struct stack_gosub_frame);
		f = (struct stack_gosub_frame *)spt;
		f->frame_type = STACK_GOSUB_FLAG;
		f->txtpos = txtpos;
		f->current_line = current_line;
		current_line = findline();
		goto execline;
	}
	goto syntaxerror;

next:
	// Fnd the variable name
	ignore_blanks();
	if(*txtpos < 'A' || *txtpos > 'Z')
		goto syntaxerror;
	txtpos++;
	if(!check_statement_end())
		goto syntaxerror;
	
gosub_return:
	// Now walk up the stack frames and find the frame we want, if present
	tempsp = spt;
	while(tempsp < memory+sizeof(memory)-1)
	{
		switch(tempsp[0])
		{
			case STACK_GOSUB_FLAG:
				if(table_index == KW_RETURN)
				{
					struct stack_gosub_frame *f = (struct stack_gosub_frame *)tempsp;
					current_line	= f->current_line;
					txtpos			= f->txtpos;
					spt += sizeof(struct stack_gosub_frame);
					goto run_next_statement;
				}
				// This is not the loop you are looking for... so Walk back up the stack
				tempsp += sizeof(struct stack_gosub_frame);
				break;
			case STACK_FOR_FLAG:
				// Flag, Var, Final, Step
				if(table_index == KW_NEXT)
				{
					struct stack_for_frame *f = (struct stack_for_frame *)tempsp;
					// Is the the variable we are looking for?
					if(txtpos[-1] == f->for_var)
					{
						short int *varaddr = ((short int *)variables_table) + txtpos[-1] - 'A'; 
						*varaddr = *varaddr + f->step;
						// Use a different test depending on the sign of the step increment
						if((f->step > 0 && *varaddr <= f->terminal) || (f->step < 0 && *varaddr >= f->terminal))
						{
							// We have to loop so don't pop the stack
							txtpos = f->txtpos;
							current_line = f->current_line;
							goto run_next_statement;
						}
						// We've run to the end of the loop. drop out of the loop, popping the stack
						spt = tempsp + sizeof(struct stack_for_frame);
						goto run_next_statement;
					}
				}
				// This is not the loop you are looking for... so Walk back up the stack
				tempsp += sizeof(struct stack_for_frame);
				break;
			default:
				goto stackstuffed;
		}
	}
	// Didn't find the variable we've been looking for
	goto syntaxerror;

assignment:
	{
		short int value;
		short int *var;

		if(*txtpos < 'A' || *txtpos > 'Z')
			goto syntaxerror;
		var = (short int *)variables_table + *txtpos - 'A';
		txtpos++;

		ignore_blanks();

		if (*txtpos != '=')
			goto syntaxerror;
		txtpos++;
		ignore_blanks();
		expression_error = 0;
		value = expression();
		if(expression_error)
			goto invalidexpr;
		// Check that we are at the end of the statement
		if(!check_statement_end())
			goto syntaxerror;
		*var = value;
	}
	goto run_next_statement;
poke:
	{
		short int value;
		unsigned char *address;

		// Work out where to put it
		expression_error = 0;
		value = expression();
		if(expression_error)
			goto invalidexpr;
		address = (unsigned char *)value;

		// check for a comma
		ignore_blanks();
		if (*txtpos != ',')
			goto syntaxerror;
		txtpos++;
		ignore_blanks();
		// check for a quote
		if (*txtpos == '"'){
			txtpos++;
				while(*txtpos!='"'){
					memory[(int)address]=*txtpos;
					txtpos++;
					address++;
					}
				memory[(int)address]=0x0D; // put newline at end of string
				txtpos++;
				goto run_next_statement;
				}
		// Now get the value to assign
		expression_error = 0;
		value = expression();
		if(expression_error)
			goto invalidexpr;
                memory[(int)address]=value;
		// Check that we are at the end of the statement
		if(!check_statement_end())
			goto syntaxerror;
	}
	goto run_next_statement;
setcursor:
	{
		short int value;
		short int x;
                short int y;

		// Work out where to put it
		x = checkParm();
                if (stopFlag==true)
                    goto prompt;

		// check for a comma
                checkForComma();
                if (stopFlag==true)
                    goto prompt;                

		// Now get the value to assign
		y=checkParm();
                if (stopFlag==true)
                    goto prompt;
                    
                if(x>19) x=19;
                if(x<0) x=0;
                if(y>7) y=7;
                if(y<0) y=0;
                TV.set_cursor(x*4,y*6);
		// Check that we are at the end of the statement
		if(!check_statement_end())
			goto syntaxerror;
	}
	goto run_next_statement;
shiftscreen:
	{       // shift entire screen 
		short int value;
		short int dist;
                short int dir;

		// Work out distance
		dist = checkParm();
                if (stopFlag==true)
                    goto prompt;
                    
		// check for a comma
                checkForComma();
                if (stopFlag==true)
                    goto prompt;                

		// Now get the value to assign
		dir=checkParm();
                if (stopFlag==true)
                    goto prompt;
                    
                TV.shift(dist,dir);
		// Check that we are at the end of the statement
		if(!check_statement_end())
			goto syntaxerror;
	}
	goto run_next_statement;
echoonoff:

	{
		short int value;
		unsigned int x;
                unsigned int y;

		// Work out where to put it
		x = checkParm();
                if (stopFlag==true)
                    goto prompt;
                if(x==1){
                  echo=true;
                }else{
                  echo=false;
                }
		// Check that we are at the end of the statement
		if(!check_statement_end())
			goto syntaxerror;
	}
	goto run_next_statement;

mTone:

	{
		short int value;
		unsigned int x;
                unsigned int y;

		// Work out where to put it
		x = checkParm();
                if (stopFlag==true)
                    goto prompt;

		// check for a comma
                checkForComma();
                if (stopFlag==true)
                    goto prompt;                

		// Now get the value to assign
		y=checkParm();
                if (stopFlag==true)
                    goto prompt;
                    
                TV.tone(x,y);
		// Check that we are at the end of the statement
		if(!check_statement_end())
			goto syntaxerror;
	}
	goto run_next_statement;
setpixel:
	{
		short int value;
		short int x;
                short int y;

		// Work out where to put it
		x = checkParm();
                if (stopFlag==true)
                    goto prompt;

		// check for a comma
                checkForComma();
                if (stopFlag==true)
                    goto prompt;                

		// Now get the value to assign
		y=checkParm();
                if (stopFlag==true)
                    goto prompt;
                    
                if(x>79) x=79;
                if(x<0) x=0;
                if(y>47) y=47;
                if(y<0) y=0;
                TV.set_pixel(x,y,1);
		// Check that we are at the end of the statement
		if(!check_statement_end())
			goto syntaxerror;
	}
	goto run_next_statement;
resetpixel:
	{
		short int value;
		short int x;
                short int y;

		// Work out where to put it
		x = checkParm();
                if (stopFlag==true)
                    goto prompt;

		// check for a comma
                checkForComma();
                if (stopFlag==true)
                    goto prompt;

		// Now get the value to assign
		y=checkParm();
                if (stopFlag==true)
                    goto prompt;
                    
                if(x>79) x=79;
                if(x<0) x=0;
                if(y>47) y=47;
                if(y<0) y=0;
                TV.set_pixel(x,y,0);
		// Check that we are at the end of the statement
		if(!check_statement_end())
			goto syntaxerror;
	}
	goto run_next_statement;
box:
	{
		short int value;
		short int x;
                short int y;
                short int x1;
                short int y1;
                short int color;
                
		// Work out where to put it
		x = checkParm();
                if (stopFlag==true)
                    goto prompt;
                    
		// check for a comma
		checkForComma();
                if (stopFlag==true)
                    goto prompt;

		// Now get the value to assign
		y=checkParm();
                if (stopFlag==true)
                    goto prompt;

		// check for a comma
                checkForComma();
                if (stopFlag==true)
                    goto prompt;

		// Now get the value to assign
		x1=checkParm();
                if (stopFlag==true)
                    goto prompt;

		// check for a comma
		checkForComma();
                if (stopFlag==true)
                    goto prompt;

		// Now get the value to assign
		y1=checkParm();
                if (stopFlag==true)
                    goto prompt;

		// check for a comma
		checkForComma();
                if (stopFlag==true)
                    goto prompt;

		// Now get the value to assign
		color=checkParm();                // if 0, erase box, 1 draw box, 2  will invert the box
                if (stopFlag==true)
                    goto prompt;
                if(x>79) x=79;
                if(x<0) x=0;
                if(y>47) y=47;
                if(y<0) y=0;
                if(x1>79) x1=79;
                if(x1<0) x1=0;
                if(y1>47) y1=47;
                if(y1<0) y1=0;
                switch (color)
                {
                   case 0:
                    TV.draw_rect(x,y,x1,y1,BLACK,0);
                    break;
                   case 1:
                    TV.draw_rect(x,y,x1,y1,WHITE,0);
                    break;
                   case 2:
                    TV.draw_rect(x,y,x1,y1,WHITE,INVERT);
                    break;
                   default:
                    TV.draw_rect(x,y,x1,y1,WHITE,0);
                }  
		// Check that we are at the end of the statement
		if(!check_statement_end())
			goto syntaxerror;
	}
	goto run_next_statement;
line:
	{
		short int value;
		short int x;
                short int y;
                short int x1;
                short int y1;
                short int color;
                
		// Work out where to put it
		x = checkParm();
                if (stopFlag==true)
                    goto prompt;
                    
		// check for a comma
		checkForComma();
                if (stopFlag==true)
                    goto prompt;

		// Now get the value to assign
		y=checkParm();
                if (stopFlag==true)
                    goto prompt;

		// check for a comma
                checkForComma();
                if (stopFlag==true)
                    goto prompt;

		// Now get the value to assign
		x1=checkParm();
                if (stopFlag==true)
                    goto prompt;

		// check for a comma
		checkForComma();
                if (stopFlag==true)
                    goto prompt;

		// Now get the value to assign
		y1=checkParm();
                if (stopFlag==true)
                    goto prompt;

		// check for a comma
		checkForComma();
                if (stopFlag==true)
                    goto prompt;

		// Now get the value to assign
		color=checkParm();                // if 0, erase box, 1 draw box, 2  will invert the box
                if (stopFlag==true)
                    goto prompt;
                    
                if(x>79) x=79;
                if(x<0) x=0;
                if(y>47) y=47;
                if(y<0) y=0;
                if(x1>79) x1=79;
                if(x1<0) x1=0;
                if(y1>47) y1=47;
                if(y1<0) y1=0;
                switch (color)
                {
                   case 0:
                    TV.draw_line(x,y,x1,y1,BLACK);
                    break;
                   case 1:
                    TV.draw_line(x,y,x1,y1,WHITE);
                    break;
                   case 2:
                    TV.draw_line(x,y,x1,y1,INVERT);
                    break;
                   default:
                    TV.draw_line(x,y,x1,y1,WHITE);
                }  
		// Check that we are at the end of the statement
		if(!check_statement_end())
			goto syntaxerror;
	}
	goto run_next_statement;
circle:
	{
		short int value;
		short int x;
                short int y;
                short int radius;
                
                short int color;
                
		// Work out where to put it
		x = checkParm();
                if (stopFlag==true)
                    goto prompt;
                    
		// check for a comma
                checkForComma();
                if (stopFlag==true)
                    goto prompt;
                    
		// Now get the value to assign
		y = checkParm();
                if (stopFlag==true)
                    goto prompt;
                    
		// check for a comma
                checkForComma();
                if (stopFlag==true)
                    goto prompt;
                    
		// Now get the value to assign
		radius = checkParm();
                if (stopFlag==true)
                    goto prompt;
                    
		// check for a comma
                checkForComma();
                if (stopFlag==true)
                    goto prompt;
                    
		// Now get the value to assign
		color = checkParm();// if 0, erase box, 1 draw box, 2  will invert the box
                if (stopFlag==true)
                    goto prompt;
                    
                if(x>79) x=79;
                if(x<0) x=0;
                if(y>48) y=48;
                if(y<0) y=0;
                
                switch (color)
                {
                   case 0:
                    TV.draw_circle(x,y,radius,BLACK);
                    break;
                   case 1:
                    TV.draw_circle(x,y,radius,WHITE);
                    break;
                   case 2:
                    TV.draw_circle(x,y,radius,INVERT);
                    break;
                   default:
                    TV.draw_circle(x,y,radius,WHITE);
                }  
		// Check that we are at the end of the statement
		if(!check_statement_end())
			goto syntaxerror;
	}
	goto run_next_statement;
list:
	linenum = testnum(); // Retuns 0 if no line found.
        
        if(*txtpos == '-'){
          list_line = findline();
          printline();
          goto warmstart;
        }
        if(*txtpos == ','){
          printline();
          list_line = findline();
                    
	  while(list_line != program_end)
          printline();
          goto warmstart;
        }
        // Should be EOL
	if(txtpos[0] != NL)
		goto syntaxerror;

	// Find the line
	list_line = findline();
	while(list_line != program_end)
          printline();
	goto warmstart;

eLoad:
       // load EEPROM contents to RAM 
       {int a = 0;
        int evalue;
        program_end=program_start+EEPROM.read(1000);
        while(a<1000){
          evalue = EEPROM.read(a);
          memory[a]=evalue;       
          a++;
        }
        
        goto warmstart;
       }
eSave:
        // save memory to the EEPROM - 1000 bytes, current usable RAM
        int i;
        i=0;
        while(i<1000){
            EEPROM.write(i, memory[i]);  
            i++;
        }
        EEPROM.write(1000,int(program_end)-int(program_start));
        goto warmstart;
sprint:                
print:
	// If we have an empty list then just put out a NL
	if(*txtpos == ':' )
	{
                line_terminator();
		txtpos++;
		goto run_next_statement;
	}
	if(*txtpos == NL)
	{
		goto execnextline;
	}

	while(1)
	{
		ignore_blanks();
		if(print_quoted_string())
		{
			;
		}
		else if(*txtpos == '"' || *txtpos == '\'')
			goto syntaxerror;
		else
		{
			short int e;
			expression_error = 0;
			e = expression();
			if(expression_error){
				if(expression_error!=15)
				goto invalidexpr;
			}
			if(expression_error==15){
				;
			}
			else
			{	
				printnum(e);
			}
		}

		// At this point we have three options, a comma or a new line
		if(*txtpos == ',')
			txtpos++;	// Skip the comma and move onto the next
		else if(txtpos[0] == ';' && (txtpos[1] == NL || txtpos[1] == ':'))
		{
			txtpos++; // This has to be the end of the print - no newline
			break;
		}
		else if(check_statement_end())
		{
			line_terminator();	// The end of the print statement
			break;
		}
		else
			goto syntaxerror;	
	}
        if(outswitch==true) outswitch=false;
	goto run_next_statement;
}
/***************************************************************************/
static int checkParm(void)
{
        int value;
	expression_error = 0;
	value = expression();
	if(expression_error){
                printmsg(invalidexprmsg);
                stopFlag=true;
		return 0;}
        stopFlag=false;
	return value;
}
/***************************************************************************/
static int checkForComma(void)
{
  		ignore_blanks();
		if (*txtpos != ','){
              	  printmsg(syntaxmsg);
            	  if(current_line != (void *)0)
            	  {
                       unsigned char tmp = *txtpos;
            	       if(*txtpos != NL)
            		  *txtpos = '^';
                       list_line = current_line;
                       printline();
                       *txtpos = tmp;
            	  }
                    line_terminator();
                    stopFlag=true;
                    
                } else
               { 
      		txtpos++;
		ignore_blanks();
                stopFlag=false;
               }
}
/***************************************************************************/
static void line_terminator(void)
{
  	outchar(NL);
	outchar(CR);
}

/***********************************************************/
static unsigned char breakcheck(void)
{

  if(keyboard.available())
    return keyboard.read() == CTRLC;
   else
     return 0;

}
/***********************************************************/
static int inchar()
{
#ifdef ARDUINO
  while(1)
  {  
    if (keyboard.available()) {
      // read the next key
      return keyboard.read();
    }
}
#else
	return getch();
#endif
}

/***********************************************************/
static void outchar(unsigned char c)
  {
  #ifdef ARDUINO
  if(outswitch==true){
    Serial.write(c);
  }
  else{
    if(echo==true){
    Serial.write(c);
    }
    TV.print(c);
  }
  #else
  	putch(c);
          TV.print(c);
  #endif
  }

/***********************************************************
void printDirectory(File dir) {
   while(true) {
    
     File entry=dir.openNextFile();
      Serial.print(dir.name());
     if (!entry) {
       // no more files
       Serial.println("**nomorefiles**");
       TV.println("**nomorefiles**");
       break;
     }
     //TV.clear_screen();
     Serial.println(' ');
     Serial.println(entry.name());
     printmsgNoNL((const unsigned char *)entry.name());
     
     if (entry.isDirectory()) {
       Serial.println("/");
       printDirectory(entry);
     } else {
       // files have sizes, directories do not
       //Serial.print("\t\t");
       Serial.println(entry.size(), DEC);
       
     }
     entry.close();
   }
}
*/




#ifdef ARDUINO
/***********************************************************/
void setup()
{
  	Serial.begin(9600);	// opens serial port, sets data rate to 9600 bps
  
        // initialize the TV stuff, select font and clear the screen
        TV.begin(0,80,48);
        TV.select_font(font4x6g);
        TV.clear_screen();
        
        // setup the keyboard 
        keyboard.begin(DataPin, IRQpin);
        
        // setup the Nunchuck
        nunchuck.begin(1);
         
}
#endif

#ifndef ARDUINO
//***********************************************************/
int main()
      {
       TV.delay(2000);
       while(1)
       // this actually starts and runs the interpreter
        loop();
      }
/***********************************************************/
#endif


