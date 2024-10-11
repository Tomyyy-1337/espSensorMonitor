#include <Arduino.h>

class IntervalArray60 {
public:
	int values[60];
	int length = 0;
	unsigned long last_update = 0;
	unsigned long interval;
	unsigned long time_offset = 0;

	unsigned long sum = 0;
	unsigned long count = 0;

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
		sum = value;
		count = 1;
		length++;
	}

	void add(int value) {
		unsigned long time = millis() / 100;
		unsigned long time_diff = time - last_update + time_offset;
		if (time_diff > interval) {
			time_offset = time_diff - interval;
			last_update = time;
			if (length >= 60) {
				for (int i = 0; i <= 58; i++) {
					values[i] = values[i + 1];
				}
				length = 59;
			}
			int average = static_cast<int>(sum / count);
			values[length] = average;
			sum = value;
			count = 1;
			length++;
		} else {
			sum += static_cast<unsigned long>(value);
			count++;
		}
	}
};

class SensorData {
public:
	IntervalArray60 minuite_values = IntervalArray60(10);
	IntervalArray60 hour_values = IntervalArray60(600);
	IntervalArray60 twelve_hour_values = IntervalArray60(600 * 12);
	IntervalArray60 day_values = IntervalArray60(600 * 24);
	IntervalArray60 week_values = IntervalArray60(600 * 24 * 7);

	SensorData() {
	}

	void addSensorData(int pot_value) {
		minuite_values.add(pot_value);
		hour_values.add(pot_value);
		twelve_hour_values.add(pot_value);
		day_values.add(pot_value);
		week_values.add(pot_value);
	}

	void addSensorDataUnChecked(int pot_value) {
		minuite_values.add_unchecked(pot_value);
		hour_values.add_unchecked(pot_value);
		twelve_hour_values.add_unchecked(pot_value);
		day_values.add_unchecked(pot_value);
		week_values.add_unchecked(pot_value);
	}

	IntervalArray60 &getArray(char identifier) {
		switch (identifier) {
			case 'M':
				return minuite_values;
			case 'H':
				return hour_values;
			case 'T':
				return twelve_hour_values;
			case 'D':
				return day_values;
			case 'W':
				return week_values;
		}
		return minuite_values;
	}
};