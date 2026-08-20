#define LED_PIN 2

void dot() {
  digitalWrite(LED_PIN, HIGH);
  delay(200);
  digitalWrite(LED_PIN, LOW);
  delay(200);
}

void dash() {
  digitalWrite(LED_PIN, HIGH);
  delay(600);
  digitalWrite(LED_PIN, LOW);
  delay(200);
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  // S (...)
  dot();
  dot();
  dot();

  delay(400);

  // O (---)
  dash();
  dash();
  dash();

  delay(400);

  // S (...)
  dot();
  dot();
  dot();

  delay(2000);
}