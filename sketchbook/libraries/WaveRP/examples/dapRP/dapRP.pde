/*
 * This example plays every .WAV file it finds on the SD card in a loop
 */
#include <WaveRP.h>
#include <SdFatUtil.h>
#include <SdFat.h>

SdFat sd;           // File system object
WaveRP wave;        // This is the only wave (audio) object, since we will only play one at a time
uint8_t dirLevel;   // indent level for file/dir names    (for prettyprinting)
dir_t dirBuf;       // buffer for directory reads
char filename[13];  // current file name

/*
 * Define macro to put error messages in flash memory
 */
#define error(msg) sd.errorHalt_P(PSTR(msg))

// Function definitions (we define them here, but the code is below)
void play(SdFile &dir);

//////////////////////////////////// SETUP
void setup() {
  Serial.begin(9600);           // set up Serial library at 9600 bps for debugging

  PgmPrintln("\nWave test!");  // say we woke up!

  PgmPrint("Free RAM: ");       // This can help with debugging, running out of RAM is bad
  Serial.println(FreeRam());

  if (!sd.init(SPI_FULL_SPEED)) sd.initErrorHalt();
  PgmPrintln("Files found (* = fragmented):");

  // Print out all of the files in all the directories.
  sd.ls(LS_R);
}

//////////////////////////////////// LOOP
void loop() {
  // rewind volume working directory, root
  sd.vwd()->rewind();
  play(sd.vwd());
}

/////////////////////////////////// HELPERS
/*
 * print error message and halt
 */
void error_P(const char *str) {
  PgmPrint("Error: ");
  SerialPrint_P(str);
  sdErrorCheck();
  while(1);
}
/*
 * print error message and halt if SD I/O error, great for debugging!
 */
void sdErrorCheck(void) {
  if (!sd.card()->errorCode()) return;
  error("SD I/o error:");
}
/*
 * play recursively - possible stack overflow if subdirectories too nested
 */
void play(SdBaseFile* dir) {
  SdBaseFile file;
  while (dir->readDir(dirBuf) > 0) {    // Read every file in the directory one at a time

    // Skip it if not a subdirectory and not a .WAV file
    if (!DIR_IS_SUBDIR(&dirBuf)
         && strncmp_P((char *)&dirBuf.name[8], PSTR("WAV"), 3)) {
      continue;
    }
    SdFile::dirName(dirBuf, filename);

    Serial.println();            // clear out a new line

    for (uint8_t i = 0; i < dirLevel; i++) {
       Serial.print(' ');       // this is for prettyprinting, put spaces in front
    }
    file.close();
    if (!file.open(dir, filename, O_READ)) {        // open the file in the directory
      error("file.open failed");          // something went wrong
    }

    if (file.isDir()) {                   // check if we opened a new directory
      PgmPrint("Subdir: ");
      Serial.print(filename);
      dirLevel += 2;                      // add more spaces
      // play files in subdirectory
      play(&file);                         // recursive!
      dirLevel -= 2;
    }
    else {
      // Aha! we found a file that isnt a directory
      PgmPrint("Playing ");
      Serial.print(filename);             // print it out
      if (!wave.play(&file)) {            // Figure out, is it a WAV proper?
        PgmPrintln(" Not a valid WAV");   // ok skip it
      } else {
        Serial.println();                 // Hooray it IS a WAV proper!
        uint8_t n = 0;
        while (wave.isPlaying()) {// playing occurs in interrupts, so we print dots in realtime
          Serial.print('.');
          if (!(++n % 32))Serial.println();
          delay(100);
        }
        sdErrorCheck();                    // everything OK?
        if (wave.busyError)Serial.println(wave.busyError);     // wave decoding errors
      }
    }
  }
}