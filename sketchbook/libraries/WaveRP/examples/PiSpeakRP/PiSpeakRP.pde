/*
 * Text-to-speech example to speak the first n digits of pi.
 * The number is stored in flash, each digit is spoken one at a time.
 */
#include <SdFat.h>
#include <SdFatUtil.h>
#include <WaveRP.h>

// put pi in flash memory
const char pi[] PROGMEM = "3.1415926535897932384626433832795028841971693993751058209749";

SdFat sd;    // file system
SdFile file; // This object represent the WAV file for a pi digit or period
WaveRP wave; // This is the only wave (audio) object, since we will only play one at a time
/*
 * Define macro to put error messages in flash memory
 */
#define error(msg) sd.errorHalt_P(PSTR(msg))

//////////////////////////////////// SETUP

void setup() {
  // set up Serial library at 9600 bps
  Serial.begin(9600);           
  
  // initialize file system
  if (!sd.init()) sd.initErrorHalt();

  PgmPrintln("Files found:");
  sd.ls();
}

/////////////////////////////////// LOOP

unsigned digit = 0;

void loop() { 
  // get next character from flash memory
  char c = pgm_read_byte(&pi[digit++]);
   
  if (c == 0) {
    digit = 0;
    Serial.println();
    return;
  }
  Serial.write(c);
    
  speaknum(c);
   
  delay(10);
}
/*
 *  Play file for one character
 */
char filename[13];
void speaknum(char c) {
  
  // copy flash string for 'period' to filename
  strcpy_P(filename, PSTR("P.WAV"));
  
  if ('0' <= c && c <= '9') {
    // digit - change 'P' to digit
    filename[0] = c;
  } 
  else if (c != '.') {
    // error if not period
    return;
  }
  playcomplete(filename);
}
/*
 * Play a file and wait for it to complete
 */
void playcomplete(char *name) {
  playfile(name);

  while (wave.isPlaying()){
    // do stuff while file is playing
  }
  // see if an error occurred while playing
  if (sd.card()->errorCode()) error("SD I/O error");
}
/*
 * Open and start playing a WAV file
 */
void playfile(char *name) {
  if (wave.isPlaying()) {// already playing something, so stop it!
    wave.stop(); // stop it
  }
  file.close();  // SdFat will not open a file that is already open
  if (!file.open(name, O_READ)) {
    PgmPrint("Couldn't open file ");
    Serial.println(name); 
    return;
  }
  if (!wave.play(&file)) {
    PgmPrint("Not a valid WAV ");   
    Serial.println(name);     
    return;
  }
}
