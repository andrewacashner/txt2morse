/* This is txt2morse, by Andrew A. Cashner, August 2016. */
/* The program reads in text from a file,  */
/* converts the text to morse code, */
/* and outputs the result as a .wav file. */
/* Using Douglas Thain's sound library, */
/* http://www.nd.edu/~dthain/course/cse20211/fall2013/wavfile */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#ifndef M_PI
  #define M_PI 3.14159265358979323846
#endif

#include "wavfile.h"
#include "wavfile.c"

#define MAX_FILENAME 120
#define MAX_CHARS    55
#define MAX_CHAR_SEQ 8
#define MAX_ASCII    127

#define NUM_SAMPLES       WAVFILE_SAMPLES_PER_SECOND / 14
#define UNIT_DURATION     NUM_SAMPLES
#define MAX_SIGNAL_LENGTH NUM_SAMPLES * 16

/* Function prototypes */
void write_tone(FILE *outfile, short waveform[], int duration);
void write_silence(FILE *outfile, short waveform[], int duration);
void write_morse_char(FILE *outfile, short waveform[], int signal_code[]);
void write_morse_word(FILE *outfile, short waveform[], int char_seq[], int length);

#define WRITE_DOT        write_tone(outfile, waveform, UNIT_DURATION)
#define WRITE_DASH       write_tone(outfile, waveform, 3 * UNIT_DURATION)
#define WRITE_SIGNAL_SPC write_silence(outfile, waveform, UNIT_DURATION);
#define WRITE_CHAR_SPC   write_silence(outfile, waveform, 2 * UNIT_DURATION);
#define WRITE_WORD_SPC   write_silence(outfile, waveform, 6 * UNIT_DURATION);

/* For lookup tables */
const enum { DOT, DASH, CHAR_SPC, WORD_SPC, ENDCODE } sign_type;

int main(int argc, char *argv[]) 
{

  FILE *infile, *outfile;
  char infile_name[MAX_FILENAME], outfile_name[MAX_FILENAME];

  /* Array of numeric values for each tone's waveform */
  short waveform[MAX_SIGNAL_LENGTH];

  /* Lookup table of morse signal codes */
  int morse_table[MAX_CHARS][MAX_CHAR_SEQ + 1] = {
    {'A', DOT, DASH, ENDCODE },
    {'B', DASH, DOT, DOT, DOT, ENDCODE },
    {'C', DASH, DOT, DASH, DOT, ENDCODE },
    {'D', DASH, DOT, DOT, ENDCODE },
    {'E', DOT, ENDCODE },
    {'F', DOT, DOT, DASH, DOT, ENDCODE },
    {'G', DASH, DASH, DOT, ENDCODE },
    {'H', DOT, DOT, DOT, DOT, ENDCODE },
    {'I', DOT, DOT, ENDCODE },
    {'J', DOT, DASH, DASH, DASH, ENDCODE },
    {'K', DASH, DOT, DASH, ENDCODE },
    {'L', DOT, DASH, DOT, DOT, ENDCODE },
    {'M', DASH, DASH, ENDCODE },
    {'N', DASH, DOT, ENDCODE },
    {'O', DASH, DASH, DASH, ENDCODE },
    {'P', DOT, DASH, DASH, DOT, ENDCODE },
    {'Q', DASH, DASH, DOT, DASH, ENDCODE },
    {'R', DOT, DASH, DOT, ENDCODE },
    {'S', DOT, DOT, DOT, ENDCODE },
    {'T', DASH, ENDCODE },
    {'U', DOT, DOT, DASH, ENDCODE },
    {'V', DOT, DOT, DOT, DASH, ENDCODE },
    {'W', DOT, DASH, DASH, ENDCODE },
    {'X', DASH, DOT, DOT, DASH, ENDCODE },
    {'Y', DASH, DOT, DASH, DASH, ENDCODE },
    {'Z', DASH, DASH, DOT, DOT, ENDCODE },
    {'0', DASH, DASH, DASH, DASH, DASH, ENDCODE },
    {'1', DOT, DASH, DASH, DASH, DASH, ENDCODE },
    {'2', DOT, DOT, DASH, DASH, DASH, ENDCODE },
    {'3', DOT, DOT, DOT, DASH, DASH, ENDCODE },
    {'4', DOT, DOT, DOT, DOT, DASH, ENDCODE },
    {'5', DOT, DOT, DOT, DOT, DOT, ENDCODE },
    {'6', DASH, DOT, DOT, DOT, DOT, ENDCODE },
    {'7', DASH, DASH, DOT, DOT, DOT, ENDCODE },
    {'8', DASH, DASH, DASH, DOT, DOT, ENDCODE },
    {'9', DASH, DASH, DASH, DASH, DOT, ENDCODE },
    {'.', DOT, DASH, DOT, DASH, DOT, DASH, ENDCODE },
    {',', DASH, DASH, DOT, DOT, DASH, DASH, ENDCODE },
    {'?', DOT, DOT, DASH, DASH, DOT, DOT, ENDCODE },
    {'\'', DOT, DASH, DASH, DASH, DASH, DOT, ENDCODE },
    {'!', DASH, DOT, DASH, DOT, DASH, DASH, ENDCODE },
    {'/', DASH, DOT, DOT, DASH, DOT, ENDCODE },
    {'(', DASH, DOT, DASH, DASH, DOT, ENDCODE },
    {')', DASH, DOT, DASH, DASH, DOT, DASH, ENDCODE }, 
    {'&', DOT, DASH, DOT, DOT, DOT, ENDCODE },
    {':', DASH, DASH, DASH, DOT, DOT, DOT, ENDCODE },
    {';', DASH, DOT, DASH, DOT, DASH, DOT, ENDCODE },
    {'=', DASH, DOT, DOT, DOT, DASH, ENDCODE },
    {'+', DOT, DASH, DOT, DASH, DOT, ENDCODE },
    {'-', DASH, DOT, DOT, DOT, DOT, DASH, ENDCODE },
    {'_', DOT, DOT, DASH, DASH, DOT, DASH, ENDCODE },
    {'\"', DOT, DASH, DOT, DOT, DASH, DOT, ENDCODE },
    {'$', DOT, DOT, DOT, DASH, DOT, DOT, DASH, ENDCODE },
    {'@', DOT, DASH, DASH, DOT, DASH, DOT, ENDCODE },
    {' ', WORD_SPC, ENDCODE }
  };
  /* Lookup table indexed to ASCII codes */
  int ascii_table[MAX_ASCII];
  int morse_table_index;
  int *signal_code;

  int i, ascii_char;
  int lower_upper_ascii_difference = 'a' - 'A';
  
  /* Process options, open files for input and output from
     command-line arguments */
  if (argc != 3) {
    fprintf(stderr, "Incorrect number of arguments. "
            "Usage: txt2morse <input file> <output file>\n");
    exit(EXIT_FAILURE);
  }
  strcpy(infile_name, argv[1]);
  infile = fopen(infile_name, "r");
  if (infile == NULL) {
    fprintf(stderr, "Could not open file %s for reading.\n", infile_name);
    exit(EXIT_FAILURE);
  }
  strcpy(outfile_name, argv[2]);
  outfile = wavfile_open(outfile_name);
  if (outfile == NULL) {
    fprintf(stderr, "Could not open file %s for writing.\n", outfile_name);
    exit(EXIT_FAILURE);
  }

  /* Make lookup table to access morse codes through ASCII values */
  /* First make empty ASCII entries point to space character, which
     is last value of morse_table */
  for (i = 0; i <= MAX_ASCII; ++i) {
    ascii_char = morse_table[MAX_CHARS][0];
  }
  for (i = 0; i < MAX_CHARS; ++i) {
    ascii_char = morse_table[i][0];
    ascii_table[ascii_char] = i;
  }
  
  /* Read in characters, look up series of dots and dashes in sign
     table, output appropriate format for each dot, dash, or space. */
  while ((ascii_char = fgetc(infile)) != EOF) {
    /* Ensure valid input */
    if (ascii_char > MAX_ASCII) {
      break;
    }
    /* Ignore newlines, no processing needed */
    else if (ascii_char == '\n') {
      continue;
    }
    /* Convert lowercase to uppercase */
    else if (ascii_char >= 'a' && ascii_char <= 'z') {
      ascii_char -= lower_upper_ascii_difference; 
    }
    
    /* Get morse output patterns for each component character from
       lookup table, so 'A' -> DOT, DASH -> ". ---" */
    morse_table_index = ascii_table[ascii_char];
    signal_code = &morse_table[morse_table_index][1];
    write_morse_char(outfile, waveform, signal_code);
  }

  fclose(infile);
  wavfile_close(outfile);
  return (0);
}


void write_tone(FILE *outfile, short waveform[], int duration)
{
  int i;
  double timepoint;
  double frequency = 800.0;
  int volume = 32000;
  for (i = 0; i < duration; ++i) {
    timepoint = (double) i / WAVFILE_SAMPLES_PER_SECOND;
    waveform[i] = volume * sin(frequency * timepoint * 2 * M_PI);
  }
  wavfile_write(outfile, waveform, duration);
  return;
}
void write_silence(FILE *outfile, short waveform[], int duration)
{
  int i;
  for (i = 0; i < duration; ++i) {
    waveform[i] = 0;
  }
  wavfile_write(outfile, waveform, duration);
  return;
}
void write_morse_char(FILE *outfile, short waveform[], int signal_code[])
{
  int i;
  for (i = 0; signal_code[i] != ENDCODE; ++i) {
    if (signal_code[i] == WORD_SPC) {
      WRITE_WORD_SPC;
      break;
    } else if (signal_code[i] == DOT) {
      WRITE_DOT;
    } else {
      WRITE_DASH;
    }
    WRITE_SIGNAL_SPC;
  }
  WRITE_CHAR_SPC;
  return;
}


  
