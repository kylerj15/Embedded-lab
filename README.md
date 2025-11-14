# Arty A7 + PmodLS1 + PmodDHB1 Project

## PmodLS1 (IR Sensors)
- Connected to Pmod JA
- Sensors: 2x OPB704 (digital output)
- Wiring:

| JA Pin | Connected To |
|--------|---------------|
| JA1    | OUT1 (Sensor 1) |
| JA2    | OUT2 (Sensor 2) |
| JA7    | VCC (3.3V) |
| JA8    | GND |

- Logic : Active-Low (0 = object detected)

---

###  PmodDHB1 (Motor + Encoder)
- Connected to Pmod JD
- Motors: 2x DG01D 48:1 Mini DC Gearbox Motors
- Encoders: Hall-effect sensors (2 per motor)
- Motor Control: Direction + Enable via GPIO or PWM

---

## Vivado Block Design

- `axi_gpio_0` for reading IR sensor inputs (JA pins)
- `axi_gpio_1` or PWM IP for controlling motor direction/speed (JD pins)
- Optional: GPIO or Interrupt IP for encoder inputs
