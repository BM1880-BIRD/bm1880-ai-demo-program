+CONFIG_INPUT_EVDEV=y
+#CONFIG_INPUT_EVBUG=y
+CONFIG_INPUT_MATRIXKMAP=y
+CONFIG_KEYBOARD_GPIO=y
+CONFIG_KEYBOARD_MATRIX=y


[dts]
-----------------------------------------------------------------------------
[single key]
=============================================================================
	gpio-keys {
		compatible = "gpio-keys";
		#address-cells = <1>;
		#size-cells = <0>;

		key_1 {
			label = "KEY_1";
			linux,code = <KEY_1>;
			gpios = <&port0a 7 GPIO_ACTIVE_HIGH>;
		};
	};

------------------------------------------------------------------------------
[matrix key]	
==============================================================================
	matrix_keypad: matrix_keypad0 {
		compatible = "gpio-matrix-keypad";
		//linux,no-autorepeat;
		debounce-delay-ms = <5>;
		col-scan-delay-us = <2>;

		row-gpios = <&port0a 0 GPIO_ACTIVE_HIGH	// R0 
				&port0a 1 GPIO_ACTIVE_HIGH		// R1 
				&port0a 3 GPIO_ACTIVE_HIGH		// R2 
				&port0a 7 GPIO_ACTIVE_HIGH>;	// R3 
				

		col-gpios = <&port1a 18  GPIO_ACTIVE_HIGH 	// C0
				&port1a 19  GPIO_ACTIVE_HIGH 	// C1
				&port1a 14  GPIO_ACTIVE_HIGH 	// C2
				&port1a 15  GPIO_ACTIVE_HIGH>; 	// C3
						

		linux,keymap = <
				MATRIX_KEY(0x0, 0x0, KEY_1)
				MATRIX_KEY(0x0, 0x1, KEY_2)
				MATRIX_KEY(0x0, 0x2, KEY_3)
				MATRIX_KEY(0x0, 0x3, KEY_A)
				MATRIX_KEY(0x1, 0x0, KEY_4)
				MATRIX_KEY(0x1, 0x1, KEY_5)
				MATRIX_KEY(0x1, 0x2, KEY_6)
				MATRIX_KEY(0x1, 0x3, KEY_B)
				MATRIX_KEY(0x2, 0x0, KEY_7)
				MATRIX_KEY(0x2, 0x1, KEY_8)
				MATRIX_KEY(0x2, 0x2, KEY_9)
				MATRIX_KEY(0x2, 0x3, KEY_C)
				MATRIX_KEY(0x3, 0x0, KEY_NUMERIC_STAR) //*
				MATRIX_KEY(0x3, 0x1, KEY_0)
				MATRIX_KEY(0x3, 0x2, KEY_NUMERIC_POUND)
				MATRIX_KEY(0x3, 0x3, KEY_D)>;	
	};
