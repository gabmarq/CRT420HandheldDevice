 // Home screen 

 void movecursor (int c1, int c2, int c3, int c4) {
    tft.drawRect(20, 20, 280, 50, WHITE);
    tft.setCursor(50, 35);
    tft.setTextColor(c1, BLACK);
    tft.setTextSize(2);
    tft.print("Interval Practice");

    tft.drawRect(20, 75, 280, 50, WHITE);
    tft.setCursor(100, 90);
    tft.setTextColor(c2, BLACK);
    tft.setTextSize(2);
    tft.print("Song Mode");

    tft.drawRect(20, 130, 280, 50, WHITE);
    tft.setCursor(130, 145);
    tft.setTextColor(c3, BLACK);
    tft.setTextSize(2);
    tft.print("About");

    tft.drawRect(20, 185, 280, 50, WHITE);
    tft.setCursor(135, 200);
    tft.setTextColor(c4, BLACK);
    tft.setTextSize(2);
    tft.print("Help");
  }
