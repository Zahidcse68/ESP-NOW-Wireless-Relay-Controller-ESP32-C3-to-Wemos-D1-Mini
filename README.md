# ESP-NOW-Wireless-Relay-Controller-ESP32-C3-to-Wemos-D1-Mini
Wireless 4-channel relay control over ESP-NOW. Four push buttons on an ESP32-C3 Mini toggle four relays on a Wemos D1 mini (ESP8266). No router or Wi-Fi network needed. Includes a unicast link test that finds which MAC address the receiver acknowledges.
# ESP-NOW Wireless Relay Controller

Control four relays wirelessly with four push buttons, using ESP-NOW between an ESP32-C3 Mini (sender) and a Wemos D1 mini (receiver). Communication is direct board-to-board, with no router, internet or Wi-Fi credentials.

## Hardware
- Sender: ESP32-C3 Mini, push buttons on GPIO 5, 6, 7, 8 (to GND, internal pull-ups)
- Receiver: Wemos D1 mini, 4-channel relay module on D2, D5, D6, D7
- Both boards on Wi-Fi channel 1

## How it works
Each button press toggles one relay. The sender transmits the 4-relay state as a 4-byte packet, and also resends it every 2 seconds so the receiver recovers after a reset or a missed packet. On boot, the sender tests unicast delivery to the receiver and uses the address that acknowledges.

## Notes
- Most relay modules are active-low. Set `RELAY_ACTIVE_LOW` in the receiver if yours is active-high.
- Power the relay coils from 5 V, not the 3.3 V pin, and share GND between all boards.
