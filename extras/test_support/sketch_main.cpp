// Run the same checks as the Arduino sketch and report failure to CI.
extern bool testsPassed;
void setup();
void loop();

int main() {
  setup();
  loop();
  return testsPassed ? 0 : 1;
}
