
// Displays note

void displayNote (float readNote) {

  String noteName = whichNote(readNote);
  int noteD = noteDiff(readNote);
  
  tft.setCursor(0, 0);
  tft.setTextColor(GREEN, BLACK);
  tft.setTextSize(4);
  tft.print(" Playing: A4");
  tft.setCursor(0, 15);
  tft.println("");
  tft.print(" Harmony: C4");
  tft.setCursor(0, 100);
  tft.println("");
  tft.print(" Singing:");
  tft.print(noteName);
  tft.setCursor(0, 150);
  tft.println("");
  tft.print(" Dif:");
  tft.print(noteD);
}
