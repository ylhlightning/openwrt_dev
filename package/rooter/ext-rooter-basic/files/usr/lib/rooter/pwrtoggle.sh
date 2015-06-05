#!/bin/sh

power_toggle() {
	MODE=$1
	if [ -f "/tmp/gpiopin" ]; then
		source /tmp/gpiopin
		if [ -z $GPIOPIN2 ]; then
			echo 0 > /sys/class/gpio/gpio$GPIOPIN/value
			sleep 2
			echo 1 > /sys/class/gpio/gpio$GPIOPIN/value
		else
			if [ $MODE = 1 ]; then
				echo 0 > /sys/class/gpio/gpio$GPIOPIN/value
				sleep 2
				echo 1 > /sys/class/gpio/gpio$GPIOPIN/value
			fi
			if [ $MODE = 2 ]; then
				echo 0 > /sys/class/gpio/gpio$GPIOPIN2/value
				sleep 2
				echo 1 > /sys/class/gpio/gpio$GPIOPIN2/value
			fi
			if [ $MODE = 3 ]; then
				echo 0 > /sys/class/gpio/gpio$GPIOPIN/value
				echo 0 > /sys/class/gpio/gpio$GPIOPIN2/value
				sleep 2
				echo 1 > /sys/class/gpio/gpio$GPIOPIN/value
				echo 1 > /sys/class/gpio/gpio$GPIOPIN2/value
			fi
			sleep 2
		fi
	fi
}

power_toggle $1