## Meshtastic 
![Meshtastic](https://github.com/frenziopen/FrenziTech/blob/main/Documentation/V3%20-%20Documentation/meshtastic.png) </br>
- Meshtastic is an open source project that lets you use inexpensive LoRa radios as a long range off-grid communicator for areas without reliable cellular service.
- These devices can create a decentralized mesh network that allows users to send text messages and share their location with each other over kilometers/miles .
- They can also connect to the internet via relay nodes, enabling online communication as well.
- These are equipped with either ESP32 or nRF52 micro-controller units (MCU), which have different features and power consumption levels.
- These devices use AES256 encryption to secure the messages, and users can customize their channel settings and keys.
- Meshtastic devices have low power consumption and can last for days on a single battery or be extended with a solar cell.
- These are extensible and can be used for various applications, such as environment monitoring, heat mapping, or encrypted messaging.
- These devices also have clients for Android, iOS, Mac, Web Browser, and Linux (coming soon) , which allow users to connect and configure their devices easily.

### Limitations
- They may not be compatible with other communication systems that use different protocols or frequencies. This could limit the interoperability and scalability of your network. 
- The software may not be compatible with some devices or platforms. According to its website, Meshtastic clients are built or being built for all major desktop and mobile platforms, but some of them are still under development or not fully supported. For example, iOS and Mac apps are still in beta testing. This may limit the usability and reliability of Meshtastic software for some users.
- It uses a flooding algorithm to propagate messages across the network, which means that every node has to receive and forward every message. This may cause congestion, delay, and redundancy in the network, especially when there are many nodes or messages. Furthermore, Meshtastic does not have any routing or optimization algorithm to select the best path or relay node for each message, which may result in inefficient use of bandwidth and battery power.
