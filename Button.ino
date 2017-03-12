
#define BUTTON_PIN  4

void ButtonInit()
{
  pinMode(BUTTON_PIN, INPUT);
}

bool ButtonIsPressed()
{
  return (digitalRead(BUTTON_PIN) == LOW);  
}

