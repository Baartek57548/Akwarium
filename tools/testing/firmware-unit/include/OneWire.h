#ifndef UNIT_TEST_ONE_WIRE_H
#define UNIT_TEST_ONE_WIRE_H

class OneWire {
public:
  explicit OneWire(int pin) : pin_(pin) {}

private:
  int pin_;
};

#endif // UNIT_TEST_ONE_WIRE_H
