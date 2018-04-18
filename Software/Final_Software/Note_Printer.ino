
String whichNote (float inNote) {
  int d = 999;
  int v = 999;
  for (int i = 0; i < 36; i++) {
    int n = freq[i];
    int newD = abs((int)inNote - n);
    if (newD < d) {
      d = newD;
      v = i;
    }
  }

  return(notes[v]);
}

int noteDiff (float inNote) {
  int d = 999;
  int v = 999;
  for (int i = 0; i < 36; i++) {
    int n = freq[i];
    int newD = abs((int)inNote - n);
    if (newD < d) {
      d = newD;
      v = i;
    }
  }
  return (d);
}



