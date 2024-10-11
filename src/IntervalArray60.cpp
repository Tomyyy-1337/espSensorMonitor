#pragma once
#include <Arduino.h>

class IntervalArray60 {
public:
	int values[60];
	int length = 0;
	unsigned long last_update = 0;
	unsigned long interval;

	IntervalArray60(unsigned long interval) {
		this->interval = interval;
	}

	void add_unchecked(int value) {
		if (length >= 60) {
			for (int i = 0; i <= 58; i++) {
				values[i] = values[i + 1];
			}
			length = 59;
		}
		values[length] = value;
		length++;
	}

	void add(int value) {
		if (millis() - last_update > interval) {
			last_update = millis();
			if (length >= 60) {
				for (int i = 0; i <= 58; i++) {
					values[i] = values[i + 1];
				}
				length = 59;
			}
			values[length] = value;
			length++;
		}
	}
};